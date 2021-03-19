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

const int kStackSize = 128 * 1024; // default coroutine stack size:128 K

// per thread
class Worker : public NoCopyable {
public:
    typedef std::shared_ptr<Worker> SPtr;
    typedef std::function<void()> Func;

    Worker(Scheduler* sche,
           const std::string& name = "worker",
           int stackSize = kStackSize);
    ~Worker();

    static Worker* GetWorker();
    // add read/write IOEvent and func when IOEvent come
    // if func is nullptr, push current co into IOEvent
    static bool AddEvent(int fd, int type, Func func = nullptr);
    // remove read/write IOEvent's task and stop listen fd
    static bool DelEvent(int fd, int type); 
    // re-sche read/write IOEvent's task
    static bool HandleEvent(int fd, int type);
    // re-sche all task in IOEvent
    static bool HandleAllEvent(int fd);
    
    Scheduler* getScheduler() { return sche_; }
    static Scheduler* GetScheduler();
    const std::string& getName() { return name_; }

    // schedule and call epoll when no task
    void run();

    void idle();
    void stop();
    // notify from epoll_wait
    void notify();

    bool isIdle() { return idle_; }

public:
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
        bool needNotify = false;
        {   
            MutexGuard lock(mutex_);
            needNotify = addTaskNoLock(cf);
        }
        if (needNotify) {
            notify();
        }
    }

    template<typename InputIterator>
    void addTask(InputIterator begin, InputIterator end) {
        bool needNotify = false;
        {
            MutexGuard lock(mutex_);
            while (begin != end) {
                needNotify = addTaskNoLock(*begin) || needNotify;
                ++begin;
            }
            if (needNotify) {
                notify();
            }
        }
    }

private:
    template<typename CoroutineOrFunc>
    bool addTaskNoLock(CoroutineOrFunc cf) {
        bool needNotify = tasks_.empty() || idle_;
        Task task(cf);
        if (task.func || task.co) {
            tasks_.push_back(task);
        }
        return needNotify;
    }

    bool canStop(int64_t& timeout);

private:
    Scheduler* sche_;    
    int stackSize_;           
    std::string name_;          

    bool running_;           
    Mutex mutex_;           
    std::list<Task> tasks_;     
    bool idle_;               
    Epoller poller_;      
};


} // namespace reyao