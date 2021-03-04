#pragma once

#include "reyao/worker.h"
#include "reyao/thread.h"

namespace reyao {

class Scheduler;

class WorkerThread : public NoCopyable {
public:
    typedef std::unique_ptr<WorkerThread> UPtr;

    WorkerThread(Scheduler* scheduler,
                 const std::string& name = "worker");
    ~WorkerThread();

    Thread* getThread() { return &thread_; }
    Worker* getWorker() { return &worker_; }

private:
    Worker worker_;
    Thread thread_;
};


} //namespace reyao