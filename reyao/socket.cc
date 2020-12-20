#include "reyao/socket.h"
#include "reyao/log.h"
#include "reyao/fdmanager.h"
#include "reyao/hook.h"

#include <time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/epoll.h>

namespace reyao {

Socket::Socket(int type, int protocol)
    : type_(type),
      protocol_(protocol) {

}

Socket::~Socket() {
    if (!close()) {
        LOG_ERROR << "Socket::close" << "errno="
                  << strerror(errno);
    }
}

IPv4Address::SPtr Socket::getLocalAddr() const { 
    IPv4Address::SPtr addr = std::make_shared<IPv4Address>();
    socklen_t addrlen = addr->getAddrLen();
    if (::getsockname(sockfd_, addr->getAddr(), &addrlen)) {
        LOG_ERROR << "getsockname(" << sockfd_
                  << ") error=" << strerror(errno);
        return nullptr;
    }
    return addr;
}

IPv4Address::SPtr Socket::getPeerAddr() const { 
    IPv4Address::SPtr addr = std::make_shared<IPv4Address>();
    socklen_t addrlen = addr->getAddrLen();
    if (::getpeername(sockfd_, addr->getAddr(), &addrlen)) {
        LOG_ERROR << "getpeername(" << sockfd_
                  << ") error=" << strerror(errno);
        return nullptr;
    }
    return addr;
}

std::string Socket::toString() const {
    std::stringstream ss;
    ss << "sockfd=" << sockfd_ << " type=" << type_ 
        << " protocol=" << protocol_;
    if (listening_) {
        ss << " state=LISTEN";
        ss << " local=" << getLocalAddr()->toString();
        return ss.str();
    } else if (connected_) {
        ss << " state=CONNECTED";
        ss << " local=" << getLocalAddr()->toString()
           << " peer=" << getPeerAddr()->toString();
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
    //TODO: why % 1000 * 1000
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

bool Socket::init(int sockfd) {
    auto fdctx = g_fdmanager->getFdContext(sockfd);
    if (fdctx && fdctx->isSocketFd() &&
        !fdctx->isClose()) {
        sockfd_ = sockfd;
        connected_ = true;
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

void Socket::socket() {
    // LOG_DEBUG << "socket";
    sockfd_ = ::socket(AF_INET, type_, protocol_);
    if (sockfd_ == -1) {
        LOG_ERROR << "socket(" << type_ << ", "
                  << protocol_ << ")" << "error=" 
                  << strerror(errno);
    } else {
        setReuseAddr();
        setNoDelay();
    }
}

bool Socket::bind(const IPv4Address& addr) {
    if (!isValid()) {
        socket();
    }
    if (::bind(sockfd_, addr.getAddr(), addr.getAddrLen())) {
        LOG_ERROR << "error to bind " << addr.toString()
                  << " error=" << strerror(errno);
        return false;
    }
    return true;
}

bool Socket::listen(int backlog) {
    if (::listen(sockfd_, backlog)) {
        LOG_ERROR << "error to listen " << getLocalAddr()->toString()
                  << " error=" << strerror(errno);
        return false;
    }
    listening_ = true;
    return true;
}

Socket::SPtr Socket::accept() {
    int new_conn_fd = ::accept(sockfd_, nullptr, nullptr);
    if (new_conn_fd == -1) {
        LOG_ERROR << "accept(" << sockfd_
                  << ") errno=" << strerror(errno);
        return nullptr; 
    }
    Socket::SPtr new_conn(new Socket(type_, protocol_));
    if (new_conn->init(new_conn_fd)) {
        return new_conn;
    }
    return nullptr;
}

bool Socket::close() {
    if (sockfd_ == -1 && !isConnected()) {
        return true;
    }
    connected_ = false;
    if (sockfd_ != -1) {
        ::close(sockfd_);
        sockfd_ = -1;
    }
    return true;
}

bool Socket::connect(const IPv4Address& addr, int64_t timeout) {
    if (!isValid()) {
        socket();
        // LOG_DEBUG << "connectfd:" << sockfd_;
        // int flag = fcntl_origin(sockfd_, F_GETFL, 0);
    }

    if (timeout == -1) {
        if (::connect(sockfd_, addr.getAddr(), addr.getAddrLen())) {
            LOG_ERROR << "error to connect " << addr.toString()
                      << " error=" << strerror(errno);
            close();
            return false;
        }
    } else {
        if (::connect_with_timeout(sockfd_, addr.getAddr(), 
                                   addr.getAddrLen(), timeout)) {
            LOG_ERROR << "error to connect " << addr.toString()
                      << " timeout=" << timeout << "ms"
                      << " error=" << strerror(errno);      
            close();
            return false; 
        }
    }
    connected_ = true;
    return true;
}

int Socket::send(const void *buf, size_t len, int flags) {
    if (isConnected()) {
        return ::send(sockfd_, buf, len, flags);
    }
    return -1;
}
    
int Socket::send(iovec *buf, int iovcnt, int flags) {
    if (isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = buf;
        msg.msg_iovlen = iovcnt;
        return ::sendmsg(sockfd_, &msg, flags);
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

int Socket::sendTo(iovec *buf, int iovcnt, const IPv4Address& to, int flags) {
    if (isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = buf;
        msg.msg_iovlen = iovcnt;
        msg.msg_name = (void*)to.getAddr();
        msg.msg_namelen = to.getAddrLen();
        return ::sendmsg(sockfd_, &msg, flags);
    }
    return -1;
}

int Socket::recv(void *buf, size_t len, int flags) {
    if (isConnected()) {
        return ::recv(sockfd_, buf, len, flags);
    }
    return -1;
}

int Socket::recv(iovec *buf, int iovcnt, int flags) {
    if (isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = buf;
        msg.msg_iovlen = iovcnt;
        return ::recvmsg(sockfd_, &msg, flags);
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
    
int Socket::recvFrom(iovec *buf, int iovcnt, const IPv4Address& from, int flags) {
    if (isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = buf;
        msg.msg_iovlen = iovcnt;
        msg.msg_name = (void*)from.getAddr();
        msg.msg_namelen = from.getAddrLen();
        return ::recvmsg(sockfd_, &msg, flags);
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
    
} //namespace reyao
