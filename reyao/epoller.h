#pragma once

#include "reyao/coroutine.h"


#include <sys/epoll.h>

#include <memory>
#include <functional>
#include <vector>
#include <atomic>

namespace reyao {


class Worker;

class Epoller {
public:
    typedef std::function<void()> Func;
    // support type:
    // NONE     --> 0x0,
    // EPOLLIN  --> 0x1
    // EPOLLOUT --> 0x4 
    struct IOEvent {
        struct EventCtx {
            Worker* worker = nullptr;     //待执行io任务所在的调度器
            Coroutine::SPtr co;           //io事件以协程的方式等待执行
            Func func;                    //io事件以回调的方式等待执行
        };

        IOEvent(int f):fd(f) {}
        EventCtx& getEventCtx(int type);
        void resetEventCtx(int type);
        //将对应事件的任务加入任务队列
        void triggleEvent(int type);

        EventCtx read_event;    //读事件
        EventCtx write_event;   //写事件
        int types = 0;          //等待事件类型
        int fd;                 //文件描述符
    };

public:
    Epoller(Worker* worker);
    ~Epoller();

    //添加io事件
    bool addEvent(int fd, int type, Func func = nullptr);
    //从epoll队列中删除io事件
    bool delEvent(int fd, int type); 
    //触发指定的待执行任务
    bool handleEvent(int fd, int type);
    //触发所有注册的待执行任务
    bool handleAllEvent(int fd);
    //等待并处理超时事件与到来事件
    void wait(epoll_event* events, int maxcnt, int timeout);
    //是否还有IO事件没触发
    bool hasEvent() const { return pending_events_ != 0; }
    //从epoll_wait中唤醒
    void notify();

    int getEventCount() const { return pending_events_; }

private:
    void resize(size_t size) {
        io_events_.resize(size);

        for (size_t i = 0; i < size; i++) {
            if (!io_events_[i]) {
                io_events_[i] = new IOEvent(i);
            }
        }
    }

private:
    Worker* worker_;
    int epfd_;
    int eventfd_;
    std::atomic<size_t> pending_events_{0};
    std::vector<IOEvent*> io_events_;
};

} //namespace reyao