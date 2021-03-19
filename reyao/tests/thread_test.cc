#include "reyao/thread.h"
#include "reyao/log.h"

#include <iostream>
#include <vector>
#include <memory>
#include <string>

using namespace reyao;

void thread_test() {
    std::vector<Thread::SPtr> threads;
    for (int i = 0; i < 10; i++) {
        Thread::SPtr thread(new Thread([i]() {
            LOG_INFO << "now in thread_" << i + 1;
        }, "thread_" + std::to_string(i + 1)));
        threads.push_back(thread);
        thread->start();
    }
    for (auto& thread : threads) {
        thread->join();
    }
}

int main(int argc, char** argv) {   
    thread_test();
    return 0;
}