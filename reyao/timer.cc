#include "reyao/timer.h"
#include "reyao/util.h"
#include "reyao/log.h"

#include <assert.h>

namespace reyao {

Timer::Timer(int64_t interval, std::function<void()> func, 
             bool recursive, TimeManager* manager)
    : manager_(manager),
      recursive_(recursive),
      interval_(interval),
      func_(func) {
    expire_ = GetCurrentMs() + interval_;
}

Timer::Timer(int64_t expire) {
    expire_ = expire;
}

bool Timer::cancel() {
    MutexGuard lock(manager_->mutex_);
    if (func_) {
        func_ = nullptr;
        auto it = manager_->timers_.find(shared_from_this());
        if (it == manager_->timers_.end()) {
            LOG_ERROR << "remove an unexisted timer";
            return false;
        }
        manager_->timers_.erase(it);
        return true;
    }
    return false;
}

bool Timer::refresh() {
    MutexGuard lock(manager_->mutex_);
    if (!func_) {
        return false;
    }
    auto it = manager_->timers_.find(shared_from_this());
    if (it == manager_->timers_.end()) {
        return false;
    }
    manager_->timers_.erase(it);
    expire_ = GetCurrentMs() + interval_;
    manager_->timers_.insert(shared_from_this());
    return true;
}

bool Timer::reset(int64_t interval, bool fromNow) {
    if (interval_ == interval && !fromNow) {
        return true;
    }
    MutexGuard lock(manager_->mutex_);
    Timer::SPtr timer = shared_from_this();
    Timer::SPtr frontTimer;
    bool atFront;
    if (!func_) {
        return false;
    }
    auto it = manager_->timers_.find(timer);
    if (it == manager_->timers_.end()) {
        return false;
    }
    manager_->timers_.erase(it);
    int64_t start = 0;
    if (fromNow) {
        start = GetCurrentMs();
    } else {
        start = expire_ - interval_;
    }
    interval_ = interval;
    expire_ = start + interval_;
    manager_->timers_.insert(timer);

    atFront = (timer == *manager_->timers_.begin());
    if (atFront) {
        manager_->needNotify_ = true;
        manager_->timerInsertAtFront();
    }

    return true;
}

bool Timer::Comparator::operator()(const Timer::SPtr& lhs, const Timer::SPtr& rhs) const {
    if (!lhs && !rhs) {
        return false;
    }
    if (!lhs) {
        return true;
    }
    if (!rhs) {
        return false;
    }
    if (lhs->expire_ < rhs->expire_) {
        return true;
    }
    if (lhs->expire_ > rhs->expire_) {
        return false;
    }
    return lhs.get() < rhs.get();
}

TimeManager::TimeManager() {

}

TimeManager::~TimeManager() {

}

Timer::SPtr TimeManager::addTimer(int64_t interval, std::function<void()> func,
                                  bool recursive) {
    bool atFront;
    Timer::SPtr timer(new Timer(interval, func, recursive, this));
    MutexGuard lock(mutex_);
    auto it = timers_.insert(timer).first;
    atFront = (it == timers_.begin());

    if (atFront) {
        timerInsertAtFront();
    }
    return timer;
}

static void OnTimer(std::weak_ptr<void> weak_cond, std::function<void()> func) {
     std::shared_ptr<void> temp = weak_cond.lock();
     if (temp) {
         func();
     }
}

Timer::SPtr TimeManager::addConditonTimer(int64_t interval, 
                                          std::function<void()> func,
                                          std::weak_ptr<void> weakCond,
                                          bool recursive) {
    return addTimer(interval, std::bind(&OnTimer, weakCond, func), recursive);
}

int64_t TimeManager::getExpire() {
    MutexGuard lock(mutex_);
    needNotify_ = false;
    if (timers_.empty()) {
        return -1;
    }

    const Timer::SPtr& lasestExpire = *timers_.begin();
    int64_t now = GetCurrentMs();
    if (now > lasestExpire->expire_) {
        return 0;
    } else {
        return lasestExpire->expire_ - now;
    }
}

void TimeManager::expiredFunctions(std::vector<std::function<void()> >& expired_funcs) {
    MutexGuard lock(mutex_);
    if (timers_.empty()) {
        return;
    }

    int64_t now = GetCurrentMs();
    std::vector<Timer::SPtr> expiredTimers;
    Timer::SPtr now_timer(new Timer(now));
    auto it = timers_.lower_bound(now_timer);

    while (it != timers_.end() && (*it)->expire_ == now) {
        ++it;
    }
    expiredTimers.insert(expiredTimers.begin(), timers_.begin(), it);
    timers_.erase(timers_.begin(), it);
    expired_funcs.reserve(expiredTimers.size());

    for (auto& timer : expiredTimers) {
        expired_funcs.push_back(timer->func_);
        if (timer->recursive_) {
            timer->expire_ = now + timer->interval_;
            timers_.insert(timer);
        } else {
            timer->func_ = nullptr;
        }
    }
}

bool TimeManager::hasTimer() {
    MutexGuard lock(mutex_);
    return !timers_.empty();
}


} // namespace reyao
