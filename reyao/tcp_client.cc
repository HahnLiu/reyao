#include "reyao/tcp_client.h"

namespace reyao {

const int kConnectMaxTimeOut = 1000;

TcpClient::TcpClient(Scheduler* sche, const IPv4Address& addr)
    : sche_(sche),
      addr_(addr) {}

void TcpClient::start() {
    conn_ = Socket::CreateTcp();
    sche_->addTask([this]() {
        if (conn_->connect(addr_, kConnectMaxTimeOut)) {
            handleConnect();
        } else {
            LOG_ERROR  << "connect err " << addr_.toString();
        }
    });
}

void TcpClient::handleConnect() {
    cb_(conn_);
}


} //namespace reyao