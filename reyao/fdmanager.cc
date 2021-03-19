#include "reyao/fdmanager.h"
#include "reyao/hook.h"

#include <sys/stat.h>
#include <sys/socket.h>

namespace reyao {

FdContext::FdContext(int fd):fd_(fd) {
    init();
}

FdContext::~FdContext() {

}

void FdContext::init() {
    struct stat fd_stat;
    if (fstat(fd_, &fd_stat) != -1) {
        isSock_ = S_ISSOCK(fd_stat.st_mode);
    }

    if (isSock_) {
        int flag = fcntl_origin(fd_, F_GETFL, 0);
        if ((flag & O_NONBLOCK) == 0) {
            fcntl_origin(fd_, F_SETFL, flag | O_NONBLOCK);
        }
        isSysNonblock_ = true;
    }
}

void FdContext::setTimeout(int type, int64_t timeout) {
    if (type == SO_RCVTIMEO) {
        recvTimeout_ = timeout;
    } else if (type == SO_SNDTIMEO) {
        sendTimeout_ = timeout;
    }
}

int64_t FdContext::getTimeout(int type) {
    if (type == SO_RCVTIMEO) {
        return recvTimeout_;
    } else {
        return sendTimeout_;
    }
}

FdManager::FdManager() {

}

std::shared_ptr<FdContext> FdManager::getFdContext(int fd) {
    assert(fd >= 0);
    ReadLock lock(rwlock_);
    if ((int)fdcontexts_.size() <= fd) {
        return nullptr;
    } else {
        return fdcontexts_[fd];
    }
}

void FdManager::addFd(int fd) {
    //TODO:
    WriteLock lock(rwlock_);
    if ((int)fdcontexts_.size() <= fd) {
        fdcontexts_.resize(fd * 1.5);
    }
    fdcontexts_[fd].reset(new FdContext(fd));
}

void FdManager::delFd(int fd) {
    WriteLock lock(rwlock_);
    if ((int)fdcontexts_.size() > fd) {
        fdcontexts_[fd].reset();
    }
}

} // namespace reyao