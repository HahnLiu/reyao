#include "reyao/tcp_server.h"

#include <assert.h>

namespace reyao {

static uint64_t s_max_recv_timeout = 30 * 1000;

TcpServer::TcpServer(Scheduler* scheduler, 
                     const std::string& name)
    : scheduler_(scheduler),
      name_(name),
      stopped_(true),
      recv_timeout_(s_max_recv_timeout) {
    listen_sock_.reset(new Socket(SOCK_STREAM, 0));
    listen_sock_->socket();
}

TcpServer::~TcpServer() {
    listen_sock_->close();
}

bool TcpServer::bind(const IPv4Address& addr) {
    int rt = listen_sock_->bind(addr);
    if (!rt) {
        LOG_ERROR << "bind error addr=" << addr.toString()
                  << " error=" << strerror(errno);
        return false;
    }
    rt = listen_sock_->listen();
    if (!rt) {
        LOG_ERROR << "listen error addr=" << addr.toString()
                  << " error=" << strerror(errno);       
        return false;
    }
    LOG_INFO << "listen_sock=" << listen_sock_->toString();
    return true;
}

void TcpServer::start() {
    if (!stopped_) {
        return;
    }
    stopped_ = false;
    scheduler_->getMainWorker()->addTask(std::bind(&TcpServer::accept,
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
