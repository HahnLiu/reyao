#include "reyao/scheduler.h"
#include "reyao/hook.h"

#include <assert.h>

namespace reyao {

Scheduler::Scheduler(int worker_num,
                     const std::string& name)
    : main_worker_(this, "worker_0"),
      name_(name),
      worker_num_(worker_num),
      index_(0) {

    worker_map_[Thread::GetThreadId()] = &main_worker_;
    if (worker_num_ != 0) {
        for (int i = 0; i < worker_num_; i++) {
            WorkerThread::UPtr wt(new WorkerThread(this, "worker_" + std::to_string(i + 1)));
            auto worker = wt->getWorker();
            workers_.push_back(worker);
            worker_map_[wt->getThread().getId()] = worker;
            threads_.push_back(std::move(wt));
        }
    } else {
        workers_.push_back(&main_worker_);
        ++worker_num_;
    }
}


Scheduler::~Scheduler() {

}

void Scheduler::start() {
    // LOG_DEBUG << "scheduler start";
    main_worker_.run();
}

void Scheduler::stop() {
    // LOG_DEBUG << "scheduler stop";
    //通知线程唤醒,处理剩余任务并退出
    
    set_hook_enable(false);

    main_worker_.stop();
    main_worker_.notify();

    if (threads_.empty()) {
        return;
    }

    for (auto& worker : workers_) {
        worker->stop();
        worker->notify();
    }
}

Worker* Scheduler::getNextWorker() {
    auto worker = workers_[index_];
    index_ = (index_ + 1) % worker_num_;
    return worker;
}



} //namespace reyao