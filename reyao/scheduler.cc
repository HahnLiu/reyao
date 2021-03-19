#include "reyao/scheduler.h"
#include "reyao/hook.h"

#include <assert.h>

namespace reyao {

Scheduler::Scheduler(int thread_num,
                     const std::string& name)
    : mainWorker_(this),
      name_(name),
      threadNum_(thread_num),
      index_(0),
      initThread_(std::bind(&Scheduler::init, this), "reyao_sche_init"),
      joinThread_(std::bind(&Scheduler::joinThread, this), "reyao_sche_join"),
      initLatch_(1),
      quitLatch_(1) {
    
    assert(thread_num >= 1);
    workerMap_[Thread::GetThreadId()] = &mainWorker_;
    if (threadNum_ == 1) {
        workers_.push_back(&mainWorker_);
    } else {
        --threadNum_;
    }
}


Scheduler::~Scheduler() {
    stop();
}

void Scheduler::init() {
    if (running_) {
        return;
    }
    for (int i = 0; i < threadNum_; i++) {
        WorkerThread::UPtr wt(new WorkerThread(this, "reyao_sche_worker_" + std::to_string(i + 1)));
        auto worker = wt->getWorker();
        assert(worker != nullptr);
        workers_.push_back(worker);
        workerMap_[wt->getThread()->getId()] = worker;
        threads_.push_back(std::move(wt));
    }
    running_ = true;
    initLatch_.countDown();
    // init thread start to work
    mainWorker_.run();
}

void Scheduler::startAsync() {
    if (running_) {
        return;
    }
    initThread_.start();
    initLatch_.wait();
}

void Scheduler::wait() {
    quitLatch_.wait();
}

void Scheduler::stop() {
    if (!running_) {
        return;
    }
    running_ = false;

    mainWorker_.stop();

    if (threads_.empty()) {
        return;
    }

    for (auto& worker : workers_) {
        worker->stop();
    }
    
    if (reyao::IsHookEnable()) {
        // stop in worker, should join all worker thread in another thread.
        joinThread_.start();
    } else {
        // stop in main thread.
        joinThread();
    }
}


void Scheduler::joinThread() {
    if (initThread_.isStart()) {
        initThread_.join();
    }
    for (auto& thread : threads_) {
        thread->getThread()->join();
    }
    quitLatch_.countDown();
}

Worker* Scheduler::getNextWorker() {
    auto worker = workers_.at(index_);
    index_ = (index_ + 1) % threadNum_;
    return worker;
}


void Scheduler::timerInsertAtFront() {
    for (auto& t : threads_) {
        Worker* w = t->getWorker();
        if (w->isIdle()) {
            w->notify();
        }
    }
}


} // namespace reyao