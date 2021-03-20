#include "reyao/tcp_client.h"

namespace reyao {

static const int s_ConnectMaxTimeOut = 10 * 1000;

TcpClient::TcpClient(Scheduler* sche, IPv4Address::SPtr addr)
    : sche_(sche),
      addr_(addr) {

}

void TcpClient::start() {
    sche_->getMainWorker()->addTask([this]() {
       conn_ = Socket::CreateTcp();
       if (conn_->connect(*addr_, s_ConnectMaxTimeOut)) {
           handleConnect(conn_);
       } else {
           LOG_ERROR  << "connect err " << addr_->toString();
       }
    });
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


} // namespace reyao
