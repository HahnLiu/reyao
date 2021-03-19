#include "reyao/workerthread.h"

namespace reyao {

WorkerThread::WorkerThread(Scheduler* sche,
              const std::string& name)
    : worker_(sche, name),
      thread_(std::bind(&Worker::run, &worker_), name) {
        
    thread_.start();
}

WorkerThread::~WorkerThread() {
    // LOG_DEBUG << thread_->getId() << " join";
    thread_.join();
}

} // namespace reyao