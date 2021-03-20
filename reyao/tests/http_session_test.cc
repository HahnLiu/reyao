#include "reyao/http/http_session.h"
#include "reyao/scheduler.h"
#include "reyao/socket.h"
#include "reyao/log.h"

#include <unistd.h>

using namespace reyao;

void server() {
    Socket::SPtr listen_sock = Socket::CreateTcp();
    auto addr = IPv4Address::CreateAddress("0.0.0.0", 3000);
    listen_sock->bind(*addr);
    listen_sock->listen();
    // listen_sock->setRecvTimeout(5000);
    LOG_INFO << listen_sock->toString();

    auto conn_sock = listen_sock->accept();
    LOG_INFO << "conn_sock=" << conn_sock->toString();
    if (conn_sock) {
        LOG_INFO << conn_sock->toString();
        HttpSession session(conn_sock);
        HttpRequest req;
        if (session.recvRequest(&req)) {
            LOG_INFO << req.toString();
        } else {
            LOG_ERROR << "bad request";
        }

    }
    Worker::GetWorker()->getScheduler()->stop();
}


void client() {
    Socket::SPtr sock = Socket::CreateTcp();
    auto addr = IPv4Address::CreateAddress("0.0.0.0", 3000);
    if (sock->connect(*addr, 2000)) {
        LOG_INFO << sock->toString();
        char buf[] = "GET /index.html?id=hahnliu#fragment HTTP/1.1\r\nHost: www.hahnliu0123.com\r\nContent-Length: 11\r\n\r\nhello world";
        // char buf[] = "HTTP/1.1 200 OK\r\nHost: www.hahnliu0123.com\r\n\r\nhello world";
        sock->send(buf, sizeof buf);
        sock->close();
    }
    Worker::GetWorker()->getScheduler()->stop();
}

int main(int argc, char** argv) {
    Scheduler sh;
    sh.startAsync();
    if (argc == 1) {
        std::cout << "to few arg";
        return 0;
    }
    std::string cmd = argv[1];
    if (cmd == "s") {
        sh.addTask(server);
    } else if (cmd == "c") {
        sh.addTask(client);
    } else {
        sh.addTask(server);
        std::cout << "unknown arg";
    }

    sh.wait();
    return 0;
}