#include "reyao/asynclog.h"

#include <sys/time.h>
#include <unistd.h>

#include <vector>
#include <iostream>

using namespace reyao;

AsyncLog logger;

static int total = 0;

void asynclog_test() {
    logger.start();

    std::vector<Thread::SPtr> threads;
	struct timeval s, e;
	gettimeofday(&s, nullptr);
    for (int i = 0; i < 3; i++)
    {
        Thread::SPtr t(new Thread([]() {
            char buf[] = "helloworldhelloworldhelloworldhelloworldhelloworldhelloworldhelloworldhelloworldhelloworldhelloworld";
            for (int i = 0; i < 1000; i++) {
                logger.append(buf, strlen(buf));
                total += 100;
            }
        }, "AsyncLogTestThread_" + std::to_string(i + 1)));
        threads.push_back(t);
        t->start();
    }
    
    for (auto& thread : threads) {
        thread->join();
    }
    
	gettimeofday(&e, nullptr);
	double sec = (e.tv_sec - s.tv_sec) + (e.tv_usec - s.tv_usec) / 1000000.0;
	double speed = total / sec / 1024 / 1024;
	std::cout << "time=" << sec << "s " << "total=" << (total / 1024 / 1024) << "mb " << "speed=" << speed << "mb/s" << "\n";
    logger.stop();
}

int main(int argc, char** argv) {   
    asynclog_test();
    return 0;
}
