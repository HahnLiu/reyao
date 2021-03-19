#include "reyao/tcp_client.h"

using namespace reyao;

int main() {
    auto addr = IPv4Address::CreateAddress("0.0.0.0", 30000); // echo server port
    Scheduler sche;
    sche.wait();
    TcpClient client(&sche, addr);
    client.setConnectCallBack([](Socket::SPtr conn) {
        char buf1[1024];
        char buf2[1024];
        int nread = 0;
        int nwrite = 0;
        while ((nread = read(STDIN_FILENO, buf1, 1024)) > 0) {
            conn->send(buf1, nread);
            if ((nwrite = conn->recv(buf2, nread)) > 0) {
                write(STDOUT_FILENO, buf2, nwrite);
                LOG_INFO << "recv " << nwrite << " bytes";
            } else if (nwrite == 0) {
                LOG_WARN << "close by peer";
                break;
            }
        }
        conn->close();
    });
    if (client.start() == false) {
        LOG_INFO << "connect " << addr->toString() << " error";
    }
    sche.wait();
}
