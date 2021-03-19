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
        // add ioEvent task to sche.
        void triggleEvent(int type);

        EventCtx readEvent;  
        EventCtx writeEvent;  
        int types = 0;     
        int fd;      
    };

public:
    Epoller(Worker* worker);
    ~Epoller();

    bool addEvent(int fd, int type, Func func = nullptr);
    bool delEvent(int fd, int type); 
    bool handleEvent(int fd, int type);
    bool handleAllEvent(int fd);
    void wait(epoll_event* events, int maxcnt, int timeout);
    bool hasEvent() const { return pendingEvents_ != 0; }
    void notify();

    int getEventCount() const { return pendingEvents_; }

private:
    void resize(size_t size) {
        ioEvents_.resize(size);

        for (size_t i = 0; i < size; i++) {
            if (!ioEvents_[i]) {
                ioEvents_[i] = new IOEvent(i);
            }
        }
    }

private:
    Worker* worker_;
    int epfd_;
    int eventfd_;
    std::atomic<size_t> pendingEvents_{0};
    // 为了加快速度，直接用 vector 存放 IO 事件
    // 且当一个 sockfd 触发事件并关闭后，也不释放内存
    // 下一个相同的 sockfd 可以直接复用，是空间换事件的考虑
    std::vector<IOEvent*> ioEvents_;
    bool polling_;
};

} // namespace reyao