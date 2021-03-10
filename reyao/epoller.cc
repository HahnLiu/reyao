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
            return read_event;
        case EPOLLOUT:
            return write_event;
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

    // 加入调度器后 IOEvent 不再保存任务对象
    if (ctx.co) {
        ctx.worker->addTask(&ctx.co);
    } else {
        ctx.worker->addTask(&ctx.func);
    }
    ctx.worker = nullptr;
}

Epoller::Epoller(Worker* worker)
    : worker_(worker) {
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
    if ((int)io_events_.size() <= fd) {
        resize(fd * 1.5);
    }
    event = io_events_[fd];
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
    epevent.data.ptr = event;   // 将 IOEvent 放入 epoll_event 中
    if (epoll_ctl(epfd_, op, fd, &epevent) == -1) {
        LOG_ERROR << "epoll_ctl(" << epfd_
                  << ", " << op << ", " << fd << ", "
                  << epevent.events << ")"
                  << " errno:" << strerror(errno);
        return false;
    }

    ++pending_events_;
    
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
    if ((int)io_events_.size() <= fd) {
        return false;    
    }
    event = io_events_[fd];
  
    if (!(event->types & type)) {
        return false;
    }

    int new_types = (event->types & ~type);
    int op = new_types ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = EPOLLET | new_types;
    epevent.data.ptr = event;
        if (epoll_ctl(epfd_, op, fd, &epevent) == -1) {
        LOG_ERROR << "epoll_ctl(" << epfd_
                  << ", " << op << ", " << fd << ", "
                  << epevent.events << ")"
                  << " errno:" << strerror(errno);
        return false;
    }

    --pending_events_;

    event->types = new_types;
    event->resetEventCtx(type);
    return true;
}

bool Epoller::handleEvent(int fd, int type) {
    IOEvent* event = nullptr;
    if ((int)io_events_.size() <= fd) {
        return false;    
    }
    event = io_events_[fd];
    // 没有注册 type 事件
    if (!(event->types & type)) {
        return false;
    }

    // 继续注册未触发事件
    int new_type = (event->types & ~type);
    int op = new_type ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = EPOLLET | new_type;
    epevent.data.ptr = event;
        if (epoll_ctl(epfd_, op, fd, &epevent) == -1) {
        LOG_ERROR << "epoll_ctl(" << epfd_
                  << ", " << op << ", " << fd << ", "
                  << epevent.events << ")"
                  << " errno:" << strerror(errno);
        return false;
    }

    event->triggleEvent(type);
    --pending_events_;
    return true;
}

bool Epoller::handleAllEvent(int fd) {
    IOEvent* event = nullptr;
    if ((int)io_events_.size() <= fd) {
        return false;    
    }
    event = io_events_[fd];

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
        --pending_events_;
    }
    if (event->types & EPOLLOUT) {
        event->triggleEvent(EPOLLOUT);
        --pending_events_;
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
            LOG_DEBUG << "epoll_wait " << timeout << " ms";
            rt = epoll_wait(epfd_, events, maxcnt, timeout);
            if (rt < 0 && errno == EINTR) {
                continue;
            } else {
                break;
            }
        }
        
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

            Epoller::IOEvent* io_event = (Epoller::IOEvent*)event.data.ptr;
            if (event.events & (EPOLLERR | EPOLLHUP)) { 
                // peer关闭，标记 socket 注册的读写 event，当作读写事件统一处理，即关闭连接
                event.events |= (EPOLLIN | EPOLLOUT) & io_event->types;
            }
            int real_events = 0;
            if (event.events & EPOLLIN) {
                real_events |= EPOLLIN;
            }
            if (event.events & EPOLLOUT) {
                real_events |= EPOLLOUT;
            }

            if ((io_event->types & real_events) == 0) {
                // io_event 的读写事件被取消，跳过处理该事件 
                continue;
            }

            int left_events = io_event->types & ~real_events;
            int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
            event.events = EPOLLET & left_events;

            if (epoll_ctl(epfd_, op, io_event->fd, &event)) {
                LOG_ERROR << "epoll_ctl(" << epfd_
                << ", " << op << ", " << io_event->fd
                << ", " << event.events << ")"
                << " errno:" << strerror(errno);
            }

            if (real_events & EPOLLIN) {
                io_event->triggleEvent(EPOLLIN);
                --pending_events_;
            }
            if (real_events & EPOLLOUT) {
                io_event->triggleEvent(EPOLLOUT);
                --pending_events_;
            }

        }
}

void Epoller::notify() {
    uint64_t one = 1;
    if (write(eventfd_, &one, sizeof(one)) != sizeof(one)) {
        LOG_ERROR << "epoller eventfd write < 8 bytes";
    }
}

} //namespace reyao