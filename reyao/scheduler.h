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
#include <assert.h>

namespace reyao {

class Scheduler : public NoCopyable,
                  public TimeManager {
public:
    Scheduler(int threadNum = 1,
              const std::string& name = "Scheduler");
    ~Scheduler();

    template<typename CoroutineOrFunc>
    void addTask(CoroutineOrFunc cf, int t = -1) {
        if (t != -1) {
            if (workerMap_.find(t) == workerMap_.end()) {
                LOG_ERROR << "addTask to invalid thread " << t;
                return;
            }
            auto worker = workerMap_[t];
            worker->addTask(cf);
        } else {
            auto worker = getNextWorker();
            assert(worker != nullptr);
            worker->addTask(cf);
        }
    }

    void startAsync();
    void wait();
    void stop();
    void joinThread();
    Worker* getNextWorker();
    Worker* getMainWorker() { return &mainWorker_; }

    virtual void timerInsertAtFront() override;

private:
    void init();

    Worker mainWorker_;
    const std::string name_;
    std::map<int, Worker*> workerMap_;
    std::vector<Worker*> workers_;
    std::vector<WorkerThread::UPtr> threads_;
    int threadNum_;
    int index_;

    Thread initThread_;
    Thread joinThread_;
    CountDownLatch initLatch_;
    CountDownLatch quitLatch_;
    
    bool running_{false};
};

} // namespace reyao
