#include "reyao/epoller.h"
#include "reyao/log.h"
#include "reyao/worker.h"
#include "reyao/scheduler.h"

#include <assert.h>
#include <unistd.h>
#include <sys/eventfd.h>

namespace reyao {

Epoller::IOEvent::EventCtx& Epoller::IOEvent::getEventCtx(int type) {
    switch (type) {
        case EPOLLIN:
            return readEvent;
        case EPOLLOUT:
            return writeEvent;
        default:
            LOG_FATAL << "getEventCtx";
            assert(false);
    }
}

void Epoller::IOEvent::resetEventCtx(int type) {
    EventCtx& ctx = getEventCtx(type);
    ctx.co.reset();
    ctx.func = nullptr;
    ctx.worker = nullptr;
}

void Epoller::IOEvent::triggleEvent(int type) {
    assert(types & type);
    types = (types & ~type);
    EventCtx& ctx = getEventCtx(type);

    if (ctx.co) {
        ctx.worker->addTask(&ctx.co);
    } else {
        ctx.worker->addTask(&ctx.func);
    }
    ctx.worker = nullptr;
}

Epoller::Epoller(Worker* worker)
    : worker_(worker),
      polling_(false) {
    epfd_ = epoll_create(5);
    assert(epfd_ >= 0);
    eventfd_ = eventfd(0, EFD_NONBLOCK);
    assert(eventfd_ >= 0);
    epoll_event epevent;
    epevent.data.fd = eventfd_;
    epevent.events = EPOLLET | EPOLLIN;
    if (epoll_ctl(epfd_, EPOLL_CTL_ADD, eventfd_, &epevent) == -1) {
        LOG_ERROR << "epoller add eventfd";
    }

}

Epoller::~Epoller() {
    close(epfd_);
    close(eventfd_);
}

bool Epoller::addEvent(int fd, int type, Func func) {
    IOEvent* event = nullptr;
    if ((int)ioEvents_.size() <= fd) {
        resize(fd * 1.5);
    }
    event = ioEvents_[fd];
    // 事件已注册
    if (event->types & type) {
        LOG_ERROR << "addEvent(" << fd
                  << ", " << type << ")"
                  << " event->types:" << event->types;
        return false;
    }
    int op = event->types ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
    epoll_event epevent;
    epevent.events = EPOLLET | event->types | type;
    epevent.data.ptr = event;
    if (epoll_ctl(epfd_, op, fd, &epevent) == -1) {
        LOG_ERROR << "epoll_ctl(" << epfd_
                  << ", " << op << ", " << fd << ", "
                  << epevent.events << ")"
                  << " errno:" << strerror(errno);
        return false;
    }

    ++pendingEvents_;
    
    event->types = (event->types | type);
    IOEvent::EventCtx& ctx = event->getEventCtx(type);
    assert(!ctx.co && !ctx.worker);
    ctx.worker = Worker::GetWorker();
    if (func) {
        ctx.func = func;
    } else {
        ctx.co = Coroutine::GetCurCoroutine();
    }
    return true;
}

bool Epoller::delEvent(int fd, int type) {
    IOEvent* event = nullptr;
    if ((int)ioEvents_.size() <= fd) {
        return false;    
    }
    event = ioEvents_[fd];
  
    if (!(event->types & type)) {
        return false;
    }

    int newTypes = (event->types & ~type);
    int op = newTypes ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = EPOLLET | newTypes;
    epevent.data.ptr = event;
        if (epoll_ctl(epfd_, op, fd, &epevent) == -1) {
        LOG_ERROR << "epoll_ctl(" << epfd_
                  << ", " << op << ", " << fd << ", "
                  << epevent.events << ")"
                  << " errno:" << strerror(errno);
        return false;
    }

    --pendingEvents_;

    event->types = newTypes;
    event->resetEventCtx(type);
    return true;
}

bool Epoller::handleEvent(int fd, int type) {
    IOEvent* event = nullptr;
    if ((int)ioEvents_.size() <= fd) {
        return false;    
    }
    event = ioEvents_[fd];
    if (!(event->types & type)) {
        return false;
    }

    int newTypes = (event->types & ~type);
    int op = newTypes ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = EPOLLET | newTypes;
    epevent.data.ptr = event;
        if (epoll_ctl(epfd_, op, fd, &epevent) == -1) {
        LOG_ERROR << "epoll_ctl(" << epfd_
                  << ", " << op << ", " << fd << ", "
                  << epevent.events << ")"
                  << " errno:" << strerror(errno);
        return false;
    }

    event->triggleEvent(type);
    --pendingEvents_;
    return true;
}

bool Epoller::handleAllEvent(int fd) {
    IOEvent* event = nullptr;
    if ((int)ioEvents_.size() <= fd) {
        return false;    
    }
    event = ioEvents_[fd];

    if (event->types == 0) {
        return false;
    }

    epoll_event epevent;
    epevent.events = 0;
    if (epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, &epevent) == -1) {
        LOG_ERROR << "epoll_ctl(" << epfd_
                  << ", EPOLL_CTL_DEL" << ", " 
                  << fd << ", " << epevent.events << ")"
                  << " errno:" << strerror(errno);
        return false;
    }


    if (event->types & EPOLLIN) {
        event->triggleEvent(EPOLLIN);
        --pendingEvents_;
    }
    if (event->types & EPOLLOUT) {
        event->triggleEvent(EPOLLOUT);
        --pendingEvents_;
    }
    return true;
}

void Epoller::wait(epoll_event* events, int maxcnt, int timeout) {
        int rt = 0;
        while (true) {
            static const int kMaxTimeout = 5000;
            if (timeout != -1) {
                timeout = (int)timeout > kMaxTimeout ?
                          kMaxTimeout : timeout;
            } else {
                timeout = kMaxTimeout;
            }
            // LOG_DEBUG << "epoll_wait " << timeout << " ms";
            polling_ = true;
            rt = epoll_wait(epfd_, events, maxcnt, timeout);
            if (rt < 0 && errno == EINTR) {
                continue;
            } else {
                break;
            }
        }
        
        polling_ = false;
        std::vector<Func> funcs;
        worker_->getScheduler()->expiredFunctions(funcs);
        if (!funcs.empty()) {
            worker_->addTask(funcs.begin(), funcs.end());
            funcs.clear();
        }

        for (int i = 0; i < rt; i++) {
            epoll_event event = events[i];
            if (event.data.fd == eventfd_) {
                uint64_t one = 1;
                int rt = 0;
                if ((rt = read(eventfd_, &one, sizeof(one))) != sizeof(one)) {
                    LOG_ERROR << "epoller eventfd read < 8 bytes:" << rt << " error:" << strerror(errno); 
                }
                continue;
            }

            Epoller::IOEvent* ioEvent = (Epoller::IOEvent*)event.data.ptr;
            if (event.events & (EPOLLERR | EPOLLHUP)) { 
                // peer关闭，标记 socket 注册的读写 event，当作读写事件统一处理，即关闭连接
                event.events |= (EPOLLIN | EPOLLOUT) & ioEvent->types;
            }
            int realEvents = 0;
            if (event.events & EPOLLIN) {
                realEvents |= EPOLLIN;
            }
            if (event.events & EPOLLOUT) {
                realEvents |= EPOLLOUT;
            }

            // cancelled
            if ((ioEvent->types & realEvents) == 0) {
                continue;
            }

            int left_events = ioEvent->types & ~realEvents;
            int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
            event.events = EPOLLET & left_events;

            if (epoll_ctl(epfd_, op, ioEvent->fd, &event)) {
                LOG_ERROR << "epoll_ctl(" << epfd_
                << ", " << op << ", " << ioEvent->fd
                << ", " << event.events << ")"
                << " errno:" << strerror(errno);
            }

            if (realEvents & EPOLLIN) {
                ioEvent->triggleEvent(EPOLLIN);
                --pendingEvents_;
            }
            if (realEvents & EPOLLOUT) {
                ioEvent->triggleEvent(EPOLLOUT);
                --pendingEvents_;
            }

        }
}

void Epoller::notify() {
    if (!polling_) {
        return;
    }
    uint64_t one = 1;
    if (write(eventfd_, &one, sizeof(one)) != sizeof(one)) {
        LOG_ERROR << "epoller eventfd write < 8 bytes";
    }
}

} // namespace reyao