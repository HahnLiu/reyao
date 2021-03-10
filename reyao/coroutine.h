#pragma once

#include "reyao/thread.h"
#include "reyao/mutex.h"
#include "reyao/stackalloc.h"

#include <ucontext.h>
#include <assert.h>

#include <memory>
#include <functional>

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

    //设置协程执行函数
    void reuse(Func func);
    //切换到当前协程
    void resume();
    //挂起当前协程
    void yield();


    uint64_t getId() { return id_; }
    State getState() { return state_; }
    void setState(Coroutine::State state) { state_ = state; }
    std::string toString(State state);

    //遇到需要等待的事间，让出执行时间
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

// wait保存协程，notify将协程添加到任务队列
class CoroutineCondition {
public:
    void wait();
    void notify();
private:
    Worker* worker_;
    Coroutine::SPtr co_;
};

} //namespace reyao