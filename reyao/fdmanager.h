#pragma once

#include "reyao/mutex.h"
#include "reyao/singleton.h"
#include "reyao/nocopyable.h"
#include "reyao/scheduler.h"

#include <memory>
#include <unordered_map>

namespace reyao {

#define g_fdmanager reyao::Singleton<reyao::FdManager>::GetInstance()

//TODO: enable_shared_from_this?
class FdContext : public std::enable_shared_from_this<FdContext> {
public:
    typedef std::shared_ptr<FdContext> SPtr;
    FdContext(int fd);
    ~FdContext();

    void init();
    bool isSocketFd() const { return is_sock_; }
    bool isClose() const { return close_; }
    void setUserNonBlock(bool flag) { is_user_nonblock_ = flag; }
    bool getUserNonBlock() const { return is_user_nonblock_; }
    void setSysNonBlock(bool flag) { is_sys_nonblock_ = flag; }
    bool getSysNonBlock() const { return is_sys_nonblock_; }
    void setTimeout(int type, int64_t timeout);
    int64_t getTimeout(int type);

private:
    bool is_sock_ = false;
    bool is_sys_nonblock_ = false;
    bool is_user_nonblock_ = false;
    bool close_ = false;
    int fd_ = -1;
    int64_t recv_timeout_ = -1;
    int64_t send_timeout_ = -1;
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

} //namespace reyao