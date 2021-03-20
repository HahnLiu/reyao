#pragma once

#include "reyao/nocopyable.h"

#include <pthread.h>

namespace reyao {

class Mutex : public NoCopyable {
public:
    Mutex();
    ~Mutex();

    void lock();
    void unlock();

    pthread_mutex_t* getMutex() { return &mutex_; }

private:
    pthread_mutex_t mutex_;
};

class MutexGuard : public NoCopyable {
public:
    explicit MutexGuard(Mutex& mutex);
    ~MutexGuard();

private:
    Mutex& mutex_;
};

class Condition : public NoCopyable {
public:
    explicit Condition(Mutex& mutex);
    ~Condition();

    void wait();
    bool waitForSeconds(int seconds);
    void notify();
    void notifyAll();

private:
    Mutex&              mutex_;
    pthread_cond_t      cond_;
};

class CountDownLatch : public NoCopyable {
public:
    explicit CountDownLatch(int count);

    void wait();
    void countDown();
    
private:
    int         count_;
    Mutex       mutex_;
    Condition   cond_;
};

class RWLock {
public:
    RWLock();
    ~RWLock();

    void readLock();
    void writeLock();
    void unlock();

private:
    pthread_rwlock_t lock_;                
};

class ReadLock {
public:
    explicit ReadLock(RWLock& rwlock);
    ~ReadLock();

private:
    RWLock& rwlock_;
};

class WriteLock {
public:
    explicit WriteLock(RWLock& rwlock);
    ~WriteLock();

private:
    RWLock& rwlock_;
};

} // namespace reyao