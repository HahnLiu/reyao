#pragma once 

#include "reyao/socket.h"
#include "reyao/bytearray.h"

#include <memory>

namespace reyao {


class SocketStream {
public:
    typedef std::shared_ptr<SocketStream> SPtr;
    SocketStream(Socket::SPtr sock, bool owner = true);
    ~SocketStream();

    int read(void* buf, size_t size);
    int read(ByteArray* ba, size_t size);
    int write(const void* buf, size_t size);
    int write(ByteArray* ba, size_t size);
    void close(); 

    Socket::SPtr getSock() const { return sock_; }
    bool isConnected() const;
private:
    Socket::SPtr sock_;
    bool owner_;
};

} //namespace reyao