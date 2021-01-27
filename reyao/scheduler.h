#pragma once

#include "reyao/coroutine.h"
#include "reyao/mutex.h"
#include "reyao/thread.h"
#include "reyao/log.h"
#include "reyao/worker.h"
#include "reyao/workerthread.h"
#include "reyao/nocopyable.h"
#include "reyao/timer.h"

#include <vector>
#include <list>
#include <memory>
#include <string>
#include <atomic>
#include <map>

namespace reyao {

class Scheduler : public NoCopyable,
                  public TimeManager {
public:
    Scheduler(int worker_num,
              const std::string& name = "scheduler");
    ~Scheduler();

    template<typename CoroutineOrFunc>
    void addTask(CoroutineOrFunc cf, int t = -1) {
        if (t != -1) {
            if (worker_map_.find(t) == worker_map_.end()) {
                LOG_ERROR << "addTask to invalid thread " << t;
                return;
            }
            auto worker = worker_map_[t];
            worker->addTask(cf);
        } else {
            auto worker = getNextWorker();
            worker->addTask(cf);
        }
    }

    void start();
    void stop();
    Worker* getNextWorker();
    Worker* getMainWorker() { return &main_worker_; }

    virtual void timerInsertAtFront() override;

private:
    Worker main_worker_;
    const std::string name_;
    std::map<int, Worker*> worker_map_;
    std::vector<Worker*> workers_;
    std::vector<WorkerThread::UPtr> threads_;
    int worker_num_;
    int index_;
};

} //namespace reyao
