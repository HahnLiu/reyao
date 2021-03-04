#include "reyao/tcp_client.h"

namespace reyao {

const int kConnectMaxTimeOut = 10 * 1000;

TcpClient::TcpClient(Scheduler* sche, IPv4Address::SPtr addr)
    : sche_(sche),
      addr_(addr) {}

bool TcpClient::start() {
    //sche_->addTask([this]() {
    //    conn_ = Socket::CreateTcp();
    //    if (conn_->connect(addr_, kConnectMaxTimeOut)) {
    //        handleConnect(conn_);
    //    } else {
    //        LOG_ERROR  << "connect err " << addr_.toString();
    //    }
    //});
    conn_ = Socket::CreateTcp();
    if (conn_->connect(*addr_, kConnectMaxTimeOut)) {
        handleConnect(conn_);
        return true;
    } else {
        return false;
    }
}

void TcpClient::stop() {
    if (conn_->isConnected()) {
        LOG_DEBUG << "TcpClient close connection";
        conn_->close();
    }
}

void TcpClient::handleConnect(Socket::SPtr conn) {
    cb_(conn);
}


} //namespace reyao
