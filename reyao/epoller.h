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
            Worker* worker = nullptr;
            Coroutine::SPtr co;
            Func func;
        };

        IOEvent(int f):fd(f) {}
        EventCtx& getEventCtx(int type);
        void resetEventCtx(int type);
        //将 IO 事件的任务加入调度器
        void triggleEvent(int type);

        EventCtx read_event;    // 读事件
        EventCtx write_event;   // 写事件
        int types = 0;          // 等待事件类型
        int fd;                 // 文件描述符
    };

public:
    Epoller(Worker* worker);
    ~Epoller();

    // 添加事件
    bool addEvent(int fd, int type, Func func = nullptr);
    // 从 io 队列中删除事件
    bool delEvent(int fd, int type); 
    // 触发指定的事件并从队列中移除
    bool handleEvent(int fd, int type);
    // 触发读写事件并从队列中移除
    bool handleAllEvent(int fd);
    // epoll_wait
    void wait(epoll_event* events, int maxcnt, int timeout);
    bool hasEvent() const { return pending_events_ != 0; }
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
    // 为了加快速度，直接用 vector 存放 IO 事件
    // 且当一个 sockfd 触发事件并关闭后，也不释放内存
    // 下一个相同的 sockfd 可以直接复用，是空间换事件的考虑
    std::vector<IOEvent*> io_events_;
};

} // namespace reyao