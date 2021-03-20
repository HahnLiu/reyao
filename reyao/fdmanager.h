#pragma once

#include "reyao/mutex.h"
#include "reyao/singleton.h"
#include "reyao/nocopyable.h"
#include "reyao/scheduler.h"

#include <memory>
#include <unordered_map>

namespace reyao {

#define g_fdmanager reyao::Singleton<reyao::FdManager>::GetInstance()

// FdContext 管理所有 sockfd，对于 sockfd，默认设置非阻塞并设置 isSysNonblock_
// 使得在协程调度器时对 sockfd 进行 socket 相关函数时可以通过协程调度和 IO 复用来模拟同步调用的结果
// 而 timeout 成员记录了对 sockfd 的超时时间，超时则不再等待事件直接返回
// 同时 isUserNonblock_ 是为了让用户可以对 sockfd 执行原生的非阻塞 IO 操作
// 即调用 IO 函数时直接返回，不再放到协程调度器中的 epoll 等待事件
class FdContext : public std::enable_shared_from_this<FdContext> {
public:
    typedef std::shared_ptr<FdContext> SPtr;
    FdContext(int fd);
    ~FdContext();

    void init();
    bool isSocketFd() const { return isSock_; }
    bool isClose() const { return close_; }
    void setUserNonBlock(bool flag) { isUserNonblock_ = flag; }
    bool getUserNonBlock() const { return isUserNonblock_; }
    void setSysNonBlock(bool flag) { isSysNonblock_ = flag; }
    bool getSysNonBlock() const { return isSysNonblock_; }
    void setTimeout(int type, int64_t timeout);
    int64_t getTimeout(int type);

private:
    bool isSock_ = false;
    bool isSysNonblock_ = false;
    bool isUserNonblock_ = false;
    bool close_ = false;
    int fd_ = -1;
    int64_t recvTimeout_ = -1;
    int64_t sendTimeout_ = -1;
};

class FdManager : public NoCopyable {
public:
   
    FdManager();

    FdContext::SPtr getFdContext(int fd);
    void addFd(int fd);
    void delFd(int fd);

private:
    RWLock rwlock_;
    std::vector<FdContext::SPtr> fdcontexts_;
};

} // namespace reyao