#include "reyao/http/http_connection.h"
#include "reyao/scheduler.h"
#include "reyao/log.h"

using namespace reyao;

void test() {
    g_logger->setLevel(LogLevel::INFO);
    auto addr = IPv4Address::CreateByName("www.baidu.com", 80);
    if (!addr) {
        LOG_ERROR << "addr errno=" << strerror(errno);
        return;
    }
    Socket::SPtr sock = Socket::CreateTcp();
    if (!sock->connect(*addr)) {
        LOG_ERROR << "connect addr:" << addr->toString()
                  << " error:" << strerror(errno);
    }
    LOG_INFO << "CONNECTION:" << sock->toString();
    HttpConnection conn(sock);
    HttpRequest req;
    LOG_INFO << "request: " << req.toString();

    conn.sendRequest(&req);

    HttpResponse rsp;
    if (!conn.recvResponse(&rsp)) {
        LOG_ERROR << "response error=" << strerror(errno);
    } else {
        LOG_INFO << "response:" << rsp.toString();
    }

    Worker::GetWorker()->getScheduler()->stop();
}

void test_chunk() {
    g_logger->setLevel(LogLevel::INFO);
    auto res = HttpConnection::DoGet("http://www.sylar.top/blog/", 10 * 1000);
    LOG_INFO << "result:\n" << "error:" << res->error
             << "\nresponse:" << (res->response ? res->response->toString() : "");

    Worker::GetWorker()->getScheduler()->stop();
}

int main(int argc, char** argv) {
    Scheduler sh(1);
    sh.addTask(test_chunk);
    return 0;
}