#include "reyao/tcp_server.h"
#include "reyao/hook.h"

#include <assert.h>

namespace reyao {

static uint64_t s_maxRecvTimeout = 30 * 1000;

TcpServer::TcpServer(Scheduler* sche, 
                     IPv4Address::SPtr addr,
                     const std::string& name)
    : sche_(sche),
      addr_(addr),
      name_(name),
      running_(false),
      recvTimeout_(s_maxRecvTimeout) {
}

TcpServer::~TcpServer() {
    running_ = false;
    listenSock_->close();
}

void TcpServer::listenAndAccpet() {
    listenSock_ = Socket::CreateTcp();
    int rt = listenSock_->bind(*addr_);
    LOG_DEBUG << addr_->toString();
    if (!rt) {
        LOG_ERROR << "bind error addr=" << addr_->toString()
                  << " error=" << strerror(errno);
    }
    rt = listenSock_->listen();
    if (!rt) {
        LOG_ERROR << "listen error addr=" << addr_->toString()
                  << " error=" << strerror(errno);       
    }
    accept();
}

// TODO: combine TcpServer::listen to start
void TcpServer::start() {
    if (running_) {
        return;
    }
    running_ = true;
    sche_->getMainWorker()->addTask(std::bind(&TcpServer::listenAndAccpet,
                                         this));
}

void TcpServer::handleClient(Socket::SPtr client) {
    LOG_INFO << "handle client";
}

void TcpServer::accept() {
    while (running_) {
        Socket::SPtr client = listenSock_->accept();
        if (client) {
            client->setRecvTimeout(recvTimeout_);
            sche_->addTask(std::bind(&TcpServer::handleClient, 
                                     this, client));
            LOG_DEBUG << "accept:" << client->toString();
        } else {
            LOG_ERROR << "accept error=" << strerror(errno);
        }
    }
}

} // namespace reyao
