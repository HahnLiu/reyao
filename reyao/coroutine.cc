#include "reyao/coroutine.h"
#include "reyao/log.h"
#include "reyao/util.h"
#include "reyao/scheduler.h"

#include <assert.h>

#include <atomic>
#include <exception>


namespace reyao {

static std::atomic<uint64_t> s_coroutineId{0};

static thread_local Coroutine::SPtr t_curCoroutine = nullptr;
static thread_local Coroutine::SPtr t_mainCoroutine = nullptr;

Coroutine::Coroutine()
    : stack_(0) {
    // LOG_DEBUG << "create main coroutine";
    state_ = RUNNING;
    getcontext(&context_);
}

Coroutine::Coroutine(Func func, size_t stack_size)
    : func_(func),
      stack_(stack_size),
      id_(++s_coroutineId) {
    // LOG_DEBUG << "create coroutine " << id_;
    assert(state_ == INIT || state_ == DONE || state_ == EXCEPT);
    state_ = INIT;
    assert(getcontext(&context_) == 0);
    context_.uc_link = nullptr;
    context_.uc_stack.ss_size = stack_.size();
    context_.uc_stack.ss_sp = stack_.top();
    makecontext(&context_, &Coroutine::Entry, 0);
}

Coroutine::~Coroutine() {
    if (stack_.size() != 0) {
        assert(state_ == DONE || state_ == INIT || state_ == EXCEPT);
        // LOG_DEBUG << "destroy coroutine " << id_;
    } else {
        assert(func_ == nullptr);
        assert(stack_.top() == nullptr);
        if (t_curCoroutine.get() == this) {
            t_curCoroutine.reset();
        }
        // LOG_DEBUG << "destroy main coroutine";
    }
}

void Coroutine::reuse(Func func) {
    assert(stack_.top() != nullptr);
    assert(state_ == INIT || state_ == DONE || state_ == EXCEPT);
    state_ = INIT;
    func_ = func;
    getcontext(&context_);
    context_.uc_link = nullptr;
    context_.uc_stack.ss_sp = stack_.top();
    context_.uc_stack.ss_size = stack_.size();
    makecontext(&context_, &Coroutine::Entry, 0);
}

void Coroutine::resume() {
    assert(state_ != RUNNING);
    state_ = RUNNING;
    SetCurCoroutine(shared_from_this());
    swapcontext(&GetMainCoroutine()->context_, &context_);
}

void Coroutine::yield() {
    SetCurCoroutine(GetMainCoroutine());
    assert(GetCurCoroutine() != nullptr);
    //main_co 要用裸指针，如果使用智能指针会使 main_co 引用次数加一，
    //协程 swapcontext 时不会析构，使得主协程最后不退出
    auto main_co = GetMainCoroutine().get();
    assert(swapcontext(&context_,
           &main_co->context_) == 0);
}

std::string Coroutine::toString(State state) {
    switch (state) {
        case State::INIT:       return "INIT";
        case State::RUNNING:    return "RUNNING";
        case State::SUSPEND:    return "SUSPEND";
        case State::DONE:       return "DONE";
        case State::EXCEPT:     return "EXECPT"; 
        default:                return "ERROR STATE";
    }
}

void Coroutine::YieldToSuspend() {
    auto cur = GetCurCoroutine().get();
    assert(cur->state_ == RUNNING);
    cur->state_ = SUSPEND;
    cur->yield();
}

Coroutine::SPtr Coroutine::InitMainCoroutine() {
    if (!t_mainCoroutine) {
        t_mainCoroutine = Coroutine::SPtr(new Coroutine);
    }
    return t_mainCoroutine;
}

void Coroutine::SetCurCoroutine(Coroutine::SPtr coroutine) {
    t_curCoroutine = coroutine;
}

Coroutine::SPtr Coroutine::GetMainCoroutine() {
    return t_mainCoroutine;
}

std::shared_ptr<Coroutine> Coroutine::GetCurCoroutine() {
    if (t_curCoroutine == nullptr) {
        t_curCoroutine = InitMainCoroutine();
    }
    return t_curCoroutine;
}

uint64_t Coroutine::GetCoroutineId() {
    if (t_curCoroutine != nullptr) {
        return t_curCoroutine->id_;
    }
    return 0;
}

void Coroutine::Entry() {
    auto cur = GetCurCoroutine();
    try {
        cur->func_();
        cur->func_ = nullptr;
        cur->state_ = DONE;
    } catch (std::exception& ex) {
        cur->state_ = EXCEPT;
        LOG_ERROR << "Coroutine except: " << ex.what();
    } catch (...) {
        cur->state_ = EXCEPT;
        LOG_ERROR << "Coroutine except";
    }

    auto raw_ptr = cur.get();
    cur.reset();
    raw_ptr->yield();
}

void CoroutineCondition::wait() {
        assert(Coroutine::GetCurCoroutine());
        worker_ = Worker::GetWorker();
        co_ = Coroutine::GetCurCoroutine();
        Coroutine::YieldToSuspend();
    }

void CoroutineCondition::notify() {
    worker_->addTask(co_);
}

} // namespace reyao