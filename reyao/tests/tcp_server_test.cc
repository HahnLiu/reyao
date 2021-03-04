#include "reyao/tcp_server.h"

using namespace reyao;


int main(int argc, char** argv) {
    Scheduler sh;
    sh.startAsync();
    auto addr = IPv4Address::CreateAddress("0.0.0.0", 30000);
    LOG_INFO << addr->toString();
    TcpServer tcp_server(&sh, addr);

    tcp_server.start();
    sh.wait();

    return 0;
}