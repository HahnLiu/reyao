#include "reyao/worker.h"
#include "reyao/hook.h"
#include "reyao/epoller.h"
#include "reyao/scheduler.h"

#include <assert.h>

namespace reyao {

static thread_local Worker* t_worker = nullptr; 
static thread_local Scheduler* t_scheduler = nullptr;

Worker::Worker(Scheduler* sche,
               const std::string& name,
               int stackSize)
    : sche_(sche),
      stackSize_(stackSize),
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

Scheduler* Worker::GetScheduler() {
    return t_scheduler;
}

void Worker::run() {
    t_worker = this;
    t_scheduler = sche_;
    Coroutine::InitMainCoroutine();
    SetHookEnable(true);
    LOG_DEBUG << "thread set hook";
    running_ = true;
    
    Coroutine::SPtr idle(new Coroutine(std::bind(&Worker::idle, this),
                         stackSize_));
    Coroutine::SPtr coFunc;

    while (true) {
        Task task;
        {
            MutexGuard lock(mutex_);
            auto it = tasks_.begin();
            while (it != tasks_.end()) {
                task = *it;
                tasks_.erase(it++); //TODO:
                break;
            }
        }

        if (task.co && 
            task.co->getState() != Coroutine::DONE &&
            task.co->getState() != Coroutine::EXCEPT) {
            task.co->resume();
            // co yield to suspend or exit.
            // if state is exit, reset task so the co can relase.
            // if state is suspend, reset task only dececemt co's use_count,
            // co would not relase if other object is holding it like IOEvent.
            task.reset();
        } else if (task.func) {
            if (coFunc) {
                coFunc->reuse(task.func);
            } else {
                coFunc.reset(new Coroutine(task.func, stackSize_));
            }
            coFunc->resume();
            if (coFunc->getState() == Coroutine::SUSPEND) {
                // if state is exit, coFunc's stack can re-use at next time.
                // if state is suspend, coFunc should reset itself and decement its use_count.
                coFunc.reset();
            }
            task.reset();
        } else {
            // thread quit.
            if (idle->getState() == Coroutine::DONE) {
                break;
            }
            idle_ = true;
            idle->resume();
            idle_ = false;
        }
    }
}

void Worker::idle() {
    const uint64_t MAX_EVENTS = 256;
    epoll_event* events = new epoll_event[MAX_EVENTS]();

    std::shared_ptr<epoll_event> sptrEvents(events, [](epoll_event* ptr) {
        delete[] ptr;
    });

    while (true) {
        int64_t timeout = 0;
        if (canStop(timeout)) {
            LOG_DEBUG << "name=" << getName()
                     << " idle exit";
            break;
        }

        poller_.wait(events, MAX_EVENTS, timeout);


        Coroutine::YieldToSuspend();
    }
}

void Worker::stop() {
    running_ = false;
    LOG_DEBUG << "worker running_:" << running_;
    notify();
}

void Worker::notify() {
    poller_.notify();
}

bool Worker::canStop(int64_t& timeout) {
    timeout = sche_->getExpire();
    bool has_task = false;
    {
        MutexGuard lock(mutex_);
        has_task = !tasks_.empty();
        
    }
    return  !running_ &&
            !has_task &&
            (timeout == -1); // ignore IOEvent listening in epoll.
}

} // namespace reyao