#include "reyao/socket.h"
#include "reyao/log.h"
#include "reyao/fdmanager.h"
#include "reyao/hook.h"

#include <time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/epoll.h>

namespace reyao {

Socket::Socket(int type, int family, int protocol)
    : type_(type),
      family_(family),
      protocol_(protocol) {

}

Socket::~Socket() {
    close();
}

IPv4Address::SPtr Socket::getLocalAddr() { 
    if (!local_) {
        IPv4Address::SPtr addr = std::make_shared<IPv4Address>();
        socklen_t addrlen = addr->getAddrLen();
        if (::getsockname(sockfd_, addr->getAddr(), &addrlen)) {
            return nullptr;
        }
        local_ = addr;
    }
    return local_;
}

IPv4Address::SPtr Socket::getPeerAddr() { 
    if (!peer_) {
        IPv4Address::SPtr addr = std::make_shared<IPv4Address>();
        socklen_t addrlen = addr->getAddrLen();
        if (::getpeername(sockfd_, addr->getAddr(), &addrlen)) {
            return nullptr;
        }
        peer_ = addr;
    }
    return peer_;
}

std::string Socket::toString() const {
    std::stringstream ss;
    ss << "sockfd=" << sockfd_ << " type=" << type_ 
        << " protocol=" << protocol_;
    if (state_ == State::LISTEN) {
        ss << " state=LISTEN";
        if (local_) {
            ss << " local=" << local_->toString();
        }
    } else if (state_ == State::CONNECTED) {
        ss << " state=CONNECTED";
        if (local_) {
            ss << " local=" << local_->toString();
        }
        if (peer_) {
            ss << " peer=" << peer_->toString();
        }
    } else {
        ss << " state=CLOSE";
    }
    return ss.str();
}

int64_t Socket::getRecvTimeout() const {
    auto fdctx = g_fdmanager->getFdContext(sockfd_);
    if (fdctx) {
        return fdctx->getTimeout(SO_RCVTIMEO);
    }
    return -1; 
}

void Socket::setRecvTimeout(int64_t timeout) {
    timeval tv;
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = timeout % 1000 * 1000;
    setOption(SOL_SOCKET, SO_RCVTIMEO, tv);
}

int64_t Socket::getSendTimeout() const {
    auto fdctx = g_fdmanager->getFdContext(sockfd_);
    if (fdctx) {
        return fdctx->getTimeout(SO_SNDTIMEO);
    }
    return -1;
}

void Socket::setSendTimeout(int64_t timeout) {
    timeval tv;
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = timeout % 1000 * 1000;
    setOption(SOL_SOCKET, SO_SNDTIMEO, tv);
}


int Socket::getError() {
    int error = 0;
    socklen_t len = sizeof(error);
    if (!getOption(SOL_SOCKET, SO_ERROR, &error, &len)) {
        return -1;
    }
    return error;
}

bool Socket::getOption(int level, int option, void *result, socklen_t *len) {
    int rt = getsockopt(sockfd_, level, option, result, (socklen_t*)len);
    if (rt) {
        LOG_ERROR << "sockfd= " << sockfd_ << " getOption(" 
                   << level << ", " << option << "error="
                   << strerror(errno);
        return false;
    } else {
        return true;
    }
}

bool Socket::setOption(int level, int option, const void *result, socklen_t len) {
    int rt = setsockopt(sockfd_, level, option, result, (socklen_t)len);
    if (rt) {
        LOG_ERROR << "sockfd= " << sockfd_ << " setOption(" 
                   << level << ", " << option << ") error="
                   << strerror(errno);  
        return false;
    } else {
        return true;
    }
}


//  init connected Socket
bool Socket::init(int sockfd) {
    auto fdctx = g_fdmanager->getFdContext(sockfd);
    if (fdctx && 
        fdctx->isSocketFd() &&
        !fdctx->isClose()) {
        sockfd_ = sockfd;
        state_ = State::CONNECTED;
        getLocalAddr();
        getPeerAddr();
        setReuseAddr();
        setNoDelay();
        return true;
    }
    return false;
}

void Socket::setReuseAddr() {
    int val = 1;
    setOption(SOL_SOCKET, SO_REUSEADDR, val);
}

void Socket::setNoDelay() {
    int val = 1;
    if (type_ == SOCK_STREAM) {
        setOption(IPPROTO_TCP, TCP_NODELAY, val);
    }
}

void Socket::newSock() {
    sockfd_ = ::socket(type_, family_, protocol_);
    if (sockfd_ == -1) {
        LOG_ERROR << "newSock error=" 
                  << strerror(errno);
    } else {
        setReuseAddr();
        setNoDelay();
    }
}

bool Socket::bind(const IPv4Address& addr) {
    if (!isValid()) {
        newSock();
        if (!isValid()) {
            return false;
        }
    }
    if (::bind(sockfd_, addr.getAddr(), addr.getAddrLen())) {
        return false;
    }
    getLocalAddr();
    return true;
}

bool Socket::listen(int backlog) {
    if (!isValid()) {
        LOG_ERROR << "listen invalid socket";
        return false;
    }
    if (::listen(sockfd_, backlog)) {
        return false;
    }
    state_ = State::LISTEN;
    return true;
}

Socket::SPtr Socket::accept() {
    int new_conn_fd = ::accept(sockfd_, nullptr, nullptr);
    if (new_conn_fd == -1) {
        return nullptr; 
    }
    Socket::SPtr new_conn(new Socket(type_, family_, protocol_));
    if (new_conn->init(new_conn_fd)) {
        return new_conn;
    }
    return nullptr;
}

bool Socket::close() {
    if (state_ == State::CLOSE && sockfd_ == -1) {
        return true;
    }
    ::close(sockfd_);
    state_ = State::CLOSE;
    sockfd_ = -1;
    return true;
}

bool Socket::connect(const IPv4Address& addr, int64_t timeout) {
    if (!isValid()) {
        newSock();
    }

    if (timeout == -1) {
        if (::connect(sockfd_, addr.getAddr(), addr.getAddrLen())) {
            close();
            return false;
        }
    } else {
        if (::connect_with_timeout(sockfd_, addr.getAddr(), 
                                   addr.getAddrLen(), timeout)) {
            close();
            return false; 
        }
    }
    state_ = State::CONNECTED;
    getLocalAddr();
    getPeerAddr();
    return true;
}

int Socket::send(const void *buf, size_t len, int flags) {
    if (isConnected()) {
        return ::send(sockfd_, buf, len, flags);
    }
    return -1;
}

int Socket::sendTo(const void *buf, size_t len, const IPv4Address& to, int flags) {
    if (isConnected()) {
        return ::sendto(sockfd_, buf, len, flags, 
                        to.getAddr(), to.getAddrLen());
    }
    return -1;
}

int Socket::recv(void *buf, size_t len, int flags) {
    if (isConnected()) {
        return ::recv(sockfd_, buf, len, flags);
    }
    return -1;
}
 
int Socket::recvFrom(void *buf, size_t len, const IPv4Address& from, int flags) {
    if (isConnected()) {
        socklen_t addrlen = from.getAddrLen();
        return ::recvfrom(sockfd_, buf, len, flags, 
                          (sockaddr*)from.getAddr(), &addrlen);
    }
    return -1;
}

bool Socket::cancelRead() {
    return Worker::HandleEvent(sockfd_, EPOLLIN);
}

bool Socket::cancelWrite() {
    return Worker::HandleEvent(sockfd_, EPOLLOUT);
}

bool Socket::cancelAll() {
    return Worker::HandleAllEvent(sockfd_);
}

Socket::SPtr Socket::CreateTcp() {
    Socket::SPtr sock(new Socket(AF_INET, SOCK_STREAM, 0));
    return sock;
}
    
} // namespace reyao
