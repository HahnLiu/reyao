#pragma once

#include "reyao/coroutine.h"
#include "reyao/nocopyable.h"
#include "reyao/mutex.h"
#include "reyao/timer.h"
#include "reyao/log.h"
#include "reyao/epoller.h"

#include <memory>
#include <string>
#include <list>

namespace reyao {

class Scheduler;

const int kStackSize = 128 * 1024; //默认分配的协程栈大小

// 线程的协程调度器
class Worker : public NoCopyable {
public:
    typedef std::shared_ptr<Worker> SPtr;
    typedef std::function<void()> Func;

    Worker(Scheduler* scheduler,
           const std::string& name = "worker",
           int stack_size = kStackSize);
    ~Worker();

    // 当前线程的调度器
    static Worker* GetWorker();
    // 添加 fd 到 IO 队列，可以指定回调，如果不指定则默认回调为当前协程
    static bool AddEvent(int fd, int type, Func func = nullptr);
    // 从 IO 队列中删除 fd 的读写事件
    static bool DelEvent(int fd, int type); 
    //触发指定读写事件并将该读写事件从 IO 队列中移除
    static bool HandleEvent(int fd, int type);
    //触发该 fd 的读写事件并从 IO 队列中移除
    static bool HandleAllEvent(int fd);
    
    Scheduler* getScheduler() { return scheduler_; }
    static Scheduler* GetScheduler();
    const std::string& getName() { return name_; }

    // 启动调度器
    void run();
    // 调度器没任务时等待用 epoll 等待 IO 事件
    void idle();
    // 停止调度器
    void stop();
    // 从 epoll 中唤醒线程
    void notify();

    bool isIdle() { return idle_; }

public:
    // 调度器的任务，可以是 co 或者 func
    struct Task {
        Func func = nullptr;
        Coroutine::SPtr co = nullptr;

        Task() {}
        Task(Func f)
            : func(f) {}
        Task(Coroutine::SPtr c)
            : co(c) {}
        Task(Func* f) {
            func.swap(*f);
        }
        Task(Coroutine::SPtr* c) {
            co.swap(*c);
        }
        
        void reset() {
            func = nullptr;
            co = nullptr;
        }
    };

    template<typename CoroutineOrFunc>
    void addTask(CoroutineOrFunc cf) {
        bool need_notify = false;
        {   
            MutexGuard lock(mutex_);
            need_notify = addTaskNoLock(cf);
        }
        if (need_notify) {
            notify();
        }
    }

    template<typename InputIterator>
    void addTask(InputIterator begin, InputIterator end) {
        bool need_notify = false;
        {
            MutexGuard lock(mutex_);
            while (begin != end) {
                need_notify = addTaskNoLock(*begin) || need_notify;
                ++begin;
            }
            if (need_notify) {
                notify();
            }
        }
    }

private:
    template<typename CoroutineOrFunc>
    bool addTaskNoLock(CoroutineOrFunc cf) {
        bool need_notify = tasks_.empty() || idle_;
        Task task(cf);
        if (task.func || task.co) {
            tasks_.push_back(task);
        }
        return need_notify;
    }

    bool canStop(int64_t& timeout);

private:
    Scheduler* scheduler_;    
    int stack_size_;           
    std::string name_;          

    bool running_;           
    Mutex mutex_;           
    std::list<Task> tasks_;     
    bool idle_;               
    Epoller poller_;      
};


} // namespace reyao