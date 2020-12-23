#pragma once

#include "reyao/socket.h"
#include "reyao/scheduler.h"
#include "reyao/address.h"

namespace reyao {


class TcpClient : public NoCopyable {
public:
    typedef std::function<void(Socket::SPtr)> ConnectCallBack;

    TcpClient(Scheduler* sche, const IPv4Address& addr);

    void start();

    void setConnectCallBack(ConnectCallBack cb) { cb_ = cb; }

private:
    void handleConnect();

private:
    //TODO: retry & reconnect

    Scheduler* sche_;
    const IPv4Address& addr_;
    Socket::SPtr conn_;
    ConnectCallBack cb_;
    bool is_conn_{false};

};

} //namespace reyao