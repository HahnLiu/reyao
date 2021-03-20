#include "all.h"
#include <iostream>
using namespace qlog;


int main(int argc, char** argv) {
    if (argc != 3) {
        perror("usage: thread_num message_per_thread");
    }
    int thread_num = atoi(argv[1]);
    int msg_per_thread = atoi(argv[2]);

    QLog::SetAppender("file");
    qlog::FileAppender::GetInstance()->setFileName("bench");
    qlog::LogFormatter::SetFormat("[%time] ");
    std::vector<std::thread> threads(thread_num);

    auto start = detail::TimeStamp::Now();
    for (auto& t : threads) {
        t = std::thread([msg_per_thread]() {
            for (int i = 0; i < msg_per_thread; i++) {
                QLOG_INFO << "HelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworldHelloworld";
            }
        });
    }
// char buf[] = "[2021-03-13 16:46:05.564280] HelloworldHelloworldHelloworldHelloworldHelloworld";
    for (auto& t : threads) {
        t.join();
    }
    auto end = detail::TimeStamp::Now();

    double total = ((int64_t)thread_num * msg_per_thread * 1030) / 1024.0 / 1024;
    double times = (end.getUsec() - start.getUsec()) / 1000.0 / 1000;
    std::cout << "end" << std::endl;
    std::cout << "total MBs:" << total
              << "MBs times:" << times
              << "s speed:" << (total / times) << "MB/s" << std::endl;
    std::cout << (thread_num * msg_per_thread / times) << "msg/sec" << std::endl;
}