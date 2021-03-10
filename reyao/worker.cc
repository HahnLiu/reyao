#include "reyao/worker.h"
#include "reyao/hook.h"
#include "reyao/epoller.h"
#include "reyao/scheduler.h"

#include <assert.h>

namespace reyao {

static thread_local Worker* t_worker = nullptr; 
static thread_local Scheduler* t_scheduler = nullptr;

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

Scheduler* Worker::GetScheduler() {
    return t_scheduler;
}

void Worker::run() {
    t_worker = this;
    t_scheduler = scheduler_;
    Coroutine::InitMainCoroutine();
    SetHookEnable(true);
    LOG_DEBUG << "thread set hook";
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
            task.co->resume();
            // 调度器执行完协程后，不应保留协程对象，如果协程主动让出
            // 则应该保存好协程对象，并放入调度器的任务队列中重新调度
            task.reset();
        } else if (task.func) {
            // 对于回调任务，调度器创建一个函数协程对象执行
            // 如果在回调中主动让出，则函数协程 reset
            // 如果回调执行完并返回，则下一个回调可以复用当前函数协程的栈
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
    // 让智能指针接管资源以自动释放
    std::shared_ptr<epoll_event> sptr_events(events, [](epoll_event* ptr) {
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
    if (idle_) {
        poller_.notify();
    }
}

bool Worker::canStop(int64_t& timeout) {
    timeout = scheduler_->getExpire();
    bool has_task = false;
    {
        MutexGuard lock(mutex_);
        has_task = !tasks_.empty();
        
    }
    LOG_FMT_DEBUG("running:%d has_task:%d timeout:%d", running_, has_task, timeout);
    return  !running_ &&
            !has_task &&
            (timeout == -1); // 退出时忽略 poller 上监听的事件，如之前 listen 的端口之类的
}

} // namespace reyao