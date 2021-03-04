#include "reyao/http/http_connectionpool.h"
#include "reyao/scheduler.h"
#include "reyao//log.h"

using namespace reyao;


int main(int argc, char** argv) {
    Scheduler sh;
    sh.startAsync();
    sh.addTask([&]() {
        HttpConnectionPool::SPtr pool(new HttpConnectionPool(
                                    "www.sylar.top", "", 80, 10,
                                    30 * 1000, 20));
        
        sh.addTimer(1000, [pool]() {
                auto res = pool->doGet("/blog/", 10 * 1000);
                LOG_INFO << res->toString();
            }, true);
    });
    sh.wait();
    return 0;
}