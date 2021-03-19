#pragma once

#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <poll.h>

namespace reyao {

bool IsHookEnable();
void SetHookEnable(bool flag);

} //namespace reyao

extern "C" {

/* TODO: 
    wait to hook:
    __pipe
    __pipe2
    __socketpair
    __lib_accept
    __lib_poll
    __select
    __lib_close
    __dup*
    __new_fclose
    gethostbyname
*/

typedef unsigned int (*sleepFunc_t)(unsigned int seconds);
extern sleepFunc_t sleep_origin;

typedef int (*usleepFunc_t)(useconds_t usec);
extern usleepFunc_t usleep_origin;

typedef int (*nanosleepFunc_t)(const struct timespec* req, struct timespec* rem);
extern nanosleepFunc_t nanosleep_origin;

typedef int (*socketFunc_t)(int domain, int type, int protocol);
extern socketFunc_t socket_origin;

typedef int (*connectFunc_t)(int sockfd, const struct sockaddr *addr, 
             socklen_t addrlen);
extern connectFunc_t connect_origin;

extern int connect_with_timeout(int sockfd, const struct sockaddr* addr,
                                     socklen_t addrlen, int64_t timeout);

typedef int (*acceptFunc_t)(int sockfd, struct sockaddr *addr, 
            socklen_t *addrlen);
extern acceptFunc_t accept_origin;

typedef int (*closeFunc_t)(int fd);
extern closeFunc_t close_origin;

typedef ssize_t (*readFunc_t)(int fd, void *buf, size_t count);
extern readFunc_t read_origin;

typedef ssize_t (*writeFunc_t)(int fd, const void *buf, size_t count);
extern writeFunc_t write_origin;

typedef ssize_t (*readvFunc_t)(int fd, const struct iovec *iov, int iovcnt);
extern readvFunc_t readv_origin;

typedef ssize_t (*writevFunc_t)(int fd, const struct iovec *iov, int iovcnt);
extern writevFunc_t writev_origin;

typedef ssize_t (*recvFunc_t)(int sockfd, void *buf, size_t len, int flags);
extern recvFunc_t recv_origin;

typedef ssize_t (*recvfromFunc_t)(int sockfd, void *buf, size_t len, int flags,
                                  struct sockaddr *src_addr, socklen_t *addrlen);
extern recvfromFunc_t recvfrom_origin;

typedef ssize_t (*recvmsgFunc_t)(int sockfd, struct msghdr *msg, int flags);
extern recvmsgFunc_t recvmsg_origin;

typedef int (*recvmmsgFunc_t)(int sockfd, struct mmsghdr *msgvec, 
                              unsigned int vlen, int flags, 
                              struct timespec *timeout);
extern recvmmsgFunc_t recvmmsg_origin;

typedef ssize_t (*sendFunc_t)(int sockfd, const void *buf, size_t len, int flags);
extern sendFunc_t send_origin;

typedef ssize_t (*sendtoFunc_t)(int sockfd, const void *buf, size_t len, int flags,
                                  const struct sockaddr *dest_addr, socklen_t addrlen);
extern sendtoFunc_t sendto_origin;

typedef ssize_t (*sendmsgFunc_t)(int sockfd, const struct msghdr *msg, int flags);
extern sendmsgFunc_t sendmsg_origin;

typedef int (*sendmmsgFunc_t)(int sockfd, struct mmsghdr *msgvec,
		                          unsigned int vlen, int flags);
extern sendmmsgFunc_t sendmmsg_origin;

typedef int (*fcntlFunc_t)(int fd, int cmd, ... /* arg */ );
extern fcntlFunc_t fcntl_origin;

typedef int (*ioctlFunc_t)(int fd, unsigned long request, ...);
extern ioctlFunc_t ioctl_origin;

typedef int (*getsockoptFunc_t)(int sockfd, int level, int optname,
             void *optval, socklen_t *optlen);
extern getsockoptFunc_t getsockopt_origin;

typedef int (*setsockoptFunc_t)(int sockfd, int level, int optname,
                                const void *optval, socklen_t optlen);
extern setsockoptFunc_t setsockopt_origin;


} // extern "C"

