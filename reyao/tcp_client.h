#pragma once

#include "reyao/socket.h"
#include "reyao/scheduler.h"
#include "reyao/address.h"

namespace reyao {


class TcpClient : public NoCopyable {
public:
    typedef std::function<void(Socket::SPtr)> ConnectCallBack;

    TcpClient(Scheduler* sche, IPv4Address::SPtr addr);
    ~TcpClient() { LOG_DEBUG << "~TcpCLient"; }

    void start();
    void stop();
    Socket::SPtr getConn() { return conn_; }

    void setConnectCallBack(ConnectCallBack cb) { cb_ = cb; }

private:
    void handleConnect(Socket::SPtr conn);

private:
    //TODO: retry & reconnect

    Scheduler* sche_;
    IPv4Address::SPtr addr_;
    Socket::SPtr conn_;
    ConnectCallBack cb_;

};

} // namespace reyao
