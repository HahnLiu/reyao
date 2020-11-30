#include "reyao/address.h"
#include "reyao/log.h"
#include "reyao/scheduler.h"

#include <iostream>

using namespace reyao;

//TODO: no test yet

void ipv4_test() {
    IPv4Address addr("127.0.0.1", 30000);
    LOG_INFO << addr.toString();
}

void getHostName() {
    //TODO: may block
    // const sockaddr_in& addr = IPv4Address::GetHostByName("www.baidu.com", 80);
    // IPv4Address ipaddr(addr);
    auto ipaddr = IPv4Address::CreateByName("www.baidu.com", 80);
    LOG_INFO << ipaddr->toString();
    Worker::GetWorker()->getScheduler()->stop();
}


int main(int argc, char** argv) {
    Scheduler sh(0);
    sh.addTask(getHostName);
    sh.start();
    return 0;
}