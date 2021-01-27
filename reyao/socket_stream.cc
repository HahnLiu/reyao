#include "reyao/socket_stream.h"

#include <assert.h>

#include <vector>

namespace reyao {

SocketStream::SocketStream(Socket::SPtr sock, bool owner)
    : sock_(sock),
      owner_(owner) {

}
SocketStream::~SocketStream() {
    if (owner_) {
        close();
    }
}

int SocketStream::read(void* buf, size_t size) {
    if (!sock_->isConnected()) {
        return -1;
    }
    return sock_->recv(buf, size);
}


//从socket中取出size的数据写入ba中
int SocketStream::read(ByteArray* ba, size_t size) {
    if (!sock_->isConnected()) {
        return -1;
    }
    const char* buf = ba->getWriteArea(size);
    assert(buf);
    int rt = sock_->recv((void*)buf, size);
    if (rt > 0) {
        ba->setWritePos(ba->getWritePos() + rt);
    }
    return rt;
}

int SocketStream::write(const void* buf, size_t size) {
    if (!sock_->isConnected()) {
        return -1;
    }
    return sock_->send(buf, size);
}


//从ba中取出size的数据写入socket中，如果可读数据小于size则写入所有可读数据
int SocketStream::write(ByteArray* ba, size_t size) {
    if (!sock_->isConnected()) {
        return -1;
    }
    const char* buf = ba->getReadArea(&size);
    assert(buf);
    int rt = sock_->send(buf, size);
    if (rt > 0) {
        ba->setReadPos(ba->getReadPos() + rt);
    }
    return rt;
}

int SocketStream::write(ByteArray* ba) {
    return write(ba, ba->getReadSize());
}

void SocketStream::close() {
    if (sock_) {
        sock_->close();
    }
}

bool SocketStream::isConnected() const {
    return sock_ && sock_->isConnected();
}

} //namespace reyao