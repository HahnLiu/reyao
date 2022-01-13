#include "reyao/mutex.h"

#include <errno.h>
#include <stdint.h>
#include <sys/time.h>

namespace reyao {

Mutex::Mutex() {
    pthread_mutex_init(&mutex_, NULL);
}

Mutex::~Mutex() { 
    pthread_mutex_destroy(&mutex_); 
}

void Mutex::lock() { 
    pthread_mutex_lock(&mutex_);
}

void Mutex::unlock() { 
    pthread_mutex_unlock(&mutex_);
}

MutexGuard::MutexGuard(Mutex& mutex): mutex_(mutex) { 
    mutex_.lock();
}

MutexGuard::~MutexGuard() { 
    mutex_.unlock();
}

Condition::Condition(Mutex& mutex): mutex_(mutex) { 
    pthread_cond_init(&cond_, NULL);
}

Condition::~Condition() { 
    pthread_cond_destroy(&cond_);
}

void Condition::wait() { 

    pthread_cond_wait(&cond_, mutex_.getMutex());
}

bool Condition::waitForSeconds(int seconds) {
    struct timespec abstime;

    clock_gettime(CLOCK_REALTIME, &abstime);

    const int64_t kNanoSecondsPerSecond = 1000000000;
    int64_t nanoseconds = static_cast<int64_t>(seconds * kNanoSecondsPerSecond);

    abstime.tv_sec += static_cast<time_t>((abstime.tv_nsec + nanoseconds) / kNanoSecondsPerSecond);
    abstime.tv_nsec = static_cast<long>((abstime.tv_nsec + nanoseconds) % kNanoSecondsPerSecond);
    return ETIMEDOUT == pthread_cond_timedwait(&cond_, mutex_.getMutex(), &abstime);
}

//最少唤醒一个线程
void Condition::notify() { 
    pthread_cond_signal(&cond_);
}

void Condition::notifyAll() { 
    pthread_cond_broadcast(&cond_);
}

CountDownLatch::CountDownLatch(int count)
    : count_(count),
      cond_(mutex_) {

}

void CountDownLatch::wait() {   
    MutexGuard lock(mutex_);
    while (count_ > 0) {   
        cond_.wait();
    }
}

void CountDownLatch::countDown() {   
    MutexGuard lock(mutex_);
    if (--count_ == 0) {   
        cond_.notifyAll();
    }
}

RWLock::RWLock() { 
    pthread_rwlock_init(&lock_, nullptr);
}

RWLock::~RWLock() { 
    pthread_rwlock_destroy(&lock_);
}

void RWLock::readLock() { 
    pthread_rwlock_rdlock(&lock_);
}

void RWLock::writeLock() { 
    pthread_rwlock_wrlock(&lock_);
}

void RWLock::unlock() { 
    pthread_rwlock_unlock(&lock_);
}

ReadLock::ReadLock(RWLock& rwlock): rwlock_(rwlock) { 
    rwlock_.readLock();
}

ReadLock::~ReadLock() { 
    rwlock_.unlock();
}

WriteLock::WriteLock(RWLock& rwlock): rwlock_(rwlock) {
    rwlock_.writeLock();
}

WriteLock::~WriteLock() {
    rwlock_.unlock();
}

} //namespace reyao