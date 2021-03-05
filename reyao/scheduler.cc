#include "reyao/scheduler.h"
#include "reyao/hook.h"

#include <assert.h>

namespace reyao {

Scheduler::Scheduler(int thread_num,
                     const std::string& name)
    : main_worker_(this, "reyao_sche_1"),
      name_(name),
      thread_num_(thread_num),
      index_(0),
      init_thread_(std::bind(&Scheduler::init, this), "reyao_sche_init"),
      join_thread_(std::bind(&Scheduler::joinThread, this), "reyao_sche_join"),
      init_latch_(1),
      quit_latch_(1) {
    
    assert(thread_num >= 1);
    worker_map_[Thread::GetThreadId()] = &main_worker_;
    if (thread_num_ == 1) {
        // 当不使用线程池时，将 init_thread 当作 IO 线程
        workers_.push_back(&main_worker_);
    } else {
        --thread_num_;
    }
}


Scheduler::~Scheduler() {
    stop();
}

void Scheduler::init() {
    if (running_) {
        return;
    }

    for (int i = 0; i < thread_num_; i++) {
        WorkerThread::UPtr wt(new WorkerThread(this, "reyao_sche_worker_" + std::to_string(i + 1)));
        auto worker = wt->getWorker();
        assert(worker != nullptr);
        workers_.push_back(worker);
        worker_map_[wt->getThread()->getId()] = worker;
        threads_.push_back(std::move(wt));
    }
    running_ = true;
    init_latch_.countDown();
    // init thread start to work
    main_worker_.run();
}

void Scheduler::startAsync() {
    if (running_) {
        return;
    }
    init_thread_.start();
    init_latch_.wait();
}

void Scheduler::wait() {
    quit_latch_.wait();
}

void Scheduler::stop() {
    if (!running_) {
        return;
    }
    running_ = false;

    main_worker_.stop();

    if (threads_.empty()) {
        return;
    }

    for (auto& worker : workers_) {
        worker->stop();
    }
    
    if (reyao::IsHookEnable()) {
        // 在 Scheduler 的工作线程中调用 stop，另起一个线程收集所有结束的工作线程
        join_thread_.start();
    } else {
        // 在主线程中调用 stop，直接回收工作线程
        joinThread();
    }
}


void Scheduler::joinThread() {
    if (init_thread_.isStart()) {
        init_thread_.join();
    }
    for (auto& thread : threads_) {
        thread->getThread()->join();
    }
    quit_latch_.countDown();
}

Worker* Scheduler::getNextWorker() {
    auto worker = workers_.at(index_);
    index_ = (index_ + 1) % thread_num_;
    return worker;
}


void Scheduler::timerInsertAtFront() {
    for (auto& t : threads_) {
        // 如果有超时的定时器，找到空闲线程处理
        Worker* w = t->getWorker();
        if (w->isIdle()) {
            w->notify();
        }
    }
}


} //namespace reyao