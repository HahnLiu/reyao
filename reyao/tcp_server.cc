#include "reyao/tcp_server.h"
#include "reyao/hook.h"

#include <assert.h>

namespace reyao {

static uint64_t s_max_recv_timeout = 30 * 1000;

TcpServer::TcpServer(Scheduler* scheduler, 
                     IPv4Address::SPtr addr,
                     const std::string& name)
    : scheduler_(scheduler),
      addr_(addr),
      name_(name),
      stopped_(true),
      recv_timeout_(s_max_recv_timeout) {
}

TcpServer::~TcpServer() {
    stopped_ = true;
    listen_sock_->close();
}

void TcpServer::listenAndAccpet() {
    listen_sock_ = Socket::CreateTcp();
    int rt = listen_sock_->bind(*addr_);
    LOG_DEBUG << addr_->toString();
    if (!rt) {
        LOG_ERROR << "bind error addr=" << addr_->toString()
                  << " error=" << strerror(errno);
    }
    rt = listen_sock_->listen();
    if (!rt) {
        LOG_ERROR << "listen error addr=" << addr_->toString()
                  << " error=" << strerror(errno);       
    }
    accept();
}

// TODO: combine TcpServer::listen to start
void TcpServer::start() {
    if (!stopped_) {
        return;
    }
    stopped_ = false;
    scheduler_->getMainWorker()->addTask(std::bind(&TcpServer::listenAndAccpet,
                                         this));
}

void TcpServer::handleClient(Socket::SPtr client) {
    LOG_INFO << "handle client";
}

void TcpServer::accept() {
    while (!stopped_) {
        Socket::SPtr client = listen_sock_->accept();
        if (client) {
            client->setRecvTimeout(recv_timeout_);
            scheduler_->addTask(std::bind(&TcpServer::handleClient, 
                                          this, client));
            LOG_DEBUG << "accept:" << client->toString();
        } else {
            LOG_ERROR << "accept error=" << strerror(errno);
        }
    }
}

} //namespace reyao
