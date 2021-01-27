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

//线程的协程调度器
class Worker : public NoCopyable {
public:
    typedef std::shared_ptr<Worker> SPtr;
    typedef std::function<void()> Func;

    Worker(Scheduler* scheduler,
           const std::string& name = "worker",
           int stack_size = kStackSize);
    ~Worker();

    //当前线程的调度器
    static Worker* GetWorker();
    //添加io事件
    static bool AddEvent(int fd, int type, Func func = nullptr);
    //删除io事件
    static bool DelEvent(int fd, int type); 
    //触发指定的待执行任务
    static bool HandleEvent(int fd, int type);
    //触发所有注册的待执行任务
    static bool HandleAllEvent(int fd);
    
    Scheduler* getScheduler() { return scheduler_; }
    const std::string& getName() { return name_; }

    //启动调度器
    void run();
    //调度器没任务时等待IO事件
    void idle();
    //停止调度器，由上层调度器池Scheduler调用
    void stop();
    //唤醒调度器
    void notify();

    bool isIdle() { return idle_; }

public:
    //调度器的任务，可以是co或者func
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

    //调度器停止应满足条件：
    //1.running_状态为false
    //2.调度器无任务
    //3.没有定时事件
    //4.poller没有等待的IO事件
    bool canStop(int64_t& timeout);

private:
    Scheduler* scheduler_;      //管理Worker的调度器池
    int stack_size_;            //调度器管理的协程栈大小
    std::string name_;          //调度器名称

    bool running_;              //运行状态
    Mutex mutex_;               //互斥锁
    std::list<Task> tasks_;     //调度器任务队列
    bool idle_;                 //调度器是否空闲
    Epoller poller_;            //epoll封装类
};


} //namespace reyao