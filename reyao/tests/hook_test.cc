#include "reyao/hook.h"
#include "reyao/scheduler.h"

#include <arpa/inet.h>

#include <iostream>

using namespace reyao;

void test_sleep() {
    Worker::GetWorker()->addTask([] () {
        sleep(4);
        LOG_INFO << "sleep 4 s";
    });
    Worker::GetWorker()->addTask([] () {
        sleep(3);
        LOG_INFO << "sleep 3 s";
    });
    Worker::GetWorker()->addTask([] () {
        sleep(2);
        LOG_INFO << "sleep 2 s";
    });

    Worker::GetWorker()->getScheduler()->stop();
}

void test_sleep1() {
    sleep(3);

    Worker::GetWorker()->getScheduler()->stop();
}

void test_sock() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "14.215.177.38", &addr.sin_addr.s_addr);

    int rt = connect(sock, (const sockaddr*)&addr, sizeof(addr));
    LOG_INFO << "connect rt=" << rt << " errnr=" << strerror(errno);

    if(rt) {
        return;
    }

    const char data[] = "GET / HTTP/1.0\r\n\r\n";
    rt = send(sock, data, sizeof(data), 0);
    LOG_INFO << "send rt=" << rt << " errnr=" << strerror(errno);

    if(rt <= 0) {
        return;
    }

    std::string buff;
    buff.resize(4096);

    rt = recv(sock, &buff[0], buff.size(), 0);
    LOG_INFO << "recv rt=" << rt << " errno=" << errno;

    if(rt <= 0) {
        return;
    }

    buff.resize(rt);
    LOG_INFO << buff;

    Worker::GetWorker()->getScheduler()->stop();
}

int main(int argc, char** argv) {
    Scheduler sh;
    sh.startAsync();
    sh.addTask(test_sleep);

    sh.wait();
    return 0;
}