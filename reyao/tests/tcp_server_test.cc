#include "reyao/tcp_server.h"

using namespace reyao;


int main(int argc, char** argv) {
    Scheduler sh(0);
    IPv4Address addr("127.0.0.1", 30000);
    LOG_INFO << addr.toString();
    TcpServer tcp_server(&sh, addr);

    tcp_server.start();
    sh.start();

    return 0;
}