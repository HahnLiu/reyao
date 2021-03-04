#include "reyao/scheduler.h"
#include "reyao/log.h"
#include "reyao/coroutine.h"

#include <unistd.h>

using namespace reyao;

std::atomic<int> count{5};

void scheduler_test() {
    LOG_INFO << "in test";
    while (--count >= 0) {
        Worker::GetWorker()->getScheduler()->addTask(scheduler_test, Thread::GetThreadId());
        return;
    }
    Worker::GetWorker()->getScheduler()->stop();
}

int main(int argc, char* argv[]) {
    Scheduler sh(3);
    sh.startAsync();
    sh.addTask(scheduler_test);
    sh.wait();
    return 0;
}