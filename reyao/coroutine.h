#pragma once

#include "reyao/thread.h"
#include "reyao/mutex.h"
#include "reyao/stackalloc.h"

#include <ucontext.h>
#include <assert.h>

#include <memory>
#include <functional>
#include <atomic>

namespace reyao {

class Worker;

class Coroutine : public std::enable_shared_from_this<Coroutine> {
public:
    enum State {
      INIT,
      RUNNING,
      SUSPEND,
      DONE,
      EXCEPT
    };

public:
    typedef std::shared_ptr<Coroutine> SPtr;
    typedef std::function<void()> Func;
    Coroutine(Func func, size_t stack_size);
    ~Coroutine();

    void reuse(Func func);
    void resume();
    void yield();


    uint64_t getId() { return id_; }
    State getState() { return state_; }
    void setState(Coroutine::State state) { state_ = state; }
    std::string toString(State state);

    static void YieldToSuspend();
    static Coroutine::SPtr InitMainCoroutine();
    static Coroutine::SPtr GetMainCoroutine();
    static Coroutine::SPtr GetCurCoroutine();
    static void SetCurCoroutine(Coroutine::SPtr coroutine);
    static uint64_t GetCoroutineId();

private:
    Coroutine();
    static void Entry();

    Func func_;
    StackAlloc stack_;
    uint64_t id_ = 0;
    ucontext_t context_;
    State state_ = INIT;
};


class CoroutineCondition {
public:
    CoroutineCondition() {}
    void wait();
    void notify();
private:
    Worker* worker_ = nullptr;
    Coroutine::SPtr co_;
};

class CoroutineSpinLock : public NoCopyable {
public:
    void lock() {
        if (--sem < 0) {
            while (sem < 0);
        }
    }
    void unlock() {
        ++sem;
    }
private:
    std::atomic<int> sem{1};
};

template <class T>
class LockGuard : public NoCopyable {
public:
    LockGuard(T& l): lock_(l) {
        lock_.lock();
    }
    ~LockGuard() {
        lock_.unlock();
    }
private:
    T& lock_;
};

} // namespace reyao