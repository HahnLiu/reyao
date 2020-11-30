#include "reyao/worker.h"
#include "reyao/hook.h"
#include "reyao/epoller.h"
#include "reyao/scheduler.h"

#include <assert.h>

namespace reyao {

static thread_local Worker* t_worker = nullptr;             //线程的调度器

Worker::Worker(Scheduler* scheduler,
               const std::string& name,
               int stack_size)
    : scheduler_(scheduler),
      stack_size_(stack_size),
      name_(name),
      running_(false),
      mutex_(),
      idle_(false),
      poller_(this) {

}

Worker::~Worker() {

}

Worker* Worker::GetWorker() {
    return t_worker;
}

bool Worker::AddEvent(int fd, int type, Func func) {
    auto worker = Worker::GetWorker();
    return worker->poller_.addEvent(fd, type, func);
}
    
bool Worker::DelEvent(int fd, int type) {
    auto worker = Worker::GetWorker();
    return worker->poller_.delEvent(fd, type);
}
    
bool Worker::HandleEvent(int fd, int type) {
    auto worker = Worker::GetWorker();
    return worker->poller_.handleEvent(fd, type);
}
    
bool Worker::HandleAllEvent(int fd) {
    auto worker = Worker::GetWorker();
    return worker->poller_.handleAllEvent(fd);
}

void Worker::run() {
    t_worker = this;
    Coroutine::InitMainCoroutine();
    set_hook_enable(true);
    // LOG_DEBUG << "thread set hook";
    running_ = true;
    
    Coroutine::SPtr idle(new Coroutine(std::bind(&Worker::idle, this),
                         stack_size_));
    Coroutine::SPtr co_func;

    while (true) {
        Task task;
        {
            MutexGuard lock(mutex_);
            auto it = tasks_.begin();
            while (it != tasks_.end()) {
                task = *it;
                tasks_.erase(it++);
                break;
            }
        }

        if (task.co && 
            task.co->getState() != Coroutine::DONE &&
            task.co->getState() != Coroutine::EXCEPT) {
            // LOG_DEBUG << "take a co";
            task.co->resume();
            task.reset();
        } else if (task.func) {
            // LOG_DEBUG << "take a func";
            if (co_func) {
                co_func->reuse(task.func);
            } else {
                co_func.reset(new Coroutine(task.func, stack_size_));
            }
            co_func->resume();
            if (co_func->getState() == Coroutine::SUSPEND) {
                co_func.reset();
            }
            task.reset();
        } else {
            // LOG_DEBUG << "no task";
            if (idle->getState() == Coroutine::DONE) {
                break;
            }
            idle_ = true;
            // LOG_DEBUG << "in idle";
            idle->resume();
            // LOG_DEBUG << "from idle";
            idle_ = false;
        }
    }
}

void Worker::idle() {
    const uint64_t MAX_EVENTS = 256;
    epoll_event* events = new epoll_event[MAX_EVENTS]();
    //让智能指针接管资源以自动释放
    std::shared_ptr<epoll_event> sptr_events(events, [](epoll_event* ptr) {
        delete[] ptr;
    });

    while (true) {
        int64_t timeout = 0;
        if (canStop(timeout)) {
            LOG_INFO << "name=" << getName()
                     << " idle exit";
            break;
        }

        poller_.wait(events, MAX_EVENTS, timeout);


        Coroutine::YieldToSuspend();
    }
}

void Worker::stop() {
    running_ = false;
}

void Worker::notify() {
    if (idle_) {
        poller_.notify();
    }
}

bool Worker::canStop(int64_t& timeout) {
    timeout = getExpire();
    bool has_task = false;
    {
        MutexGuard lock(mutex_);
        has_task = !tasks_.empty();
        
    }
    return  !running_ &&
            !has_task &&
            (timeout == -1) &&
            !poller_.hasEvent();
}


void Worker::timerInsertAtFront() {
    notify();
}

} //namespace reyao