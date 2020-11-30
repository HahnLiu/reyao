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
    expire_ = GetCurrentTime() + interval_;
}

Timer::Timer(int64_t expire) {
    expire_ = expire;
}

bool Timer::cancel() {
    if (func_) {
        func_ = nullptr; //TODO:
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
    if (!func_) {
        return false;
    }
    auto it = manager_->timers_.find(shared_from_this());
    if (it == manager_->timers_.end()) {
        return false;
    }
    manager_->timers_.erase(it);
    expire_ = GetCurrentTime() + interval_;
    manager_->timers_.insert(shared_from_this());
    return true;
}

bool Timer::reset(int64_t interval, bool from_now) {
    if (interval_ == interval && !from_now) {
        return true;
    }
    Timer::SPtr timer = shared_from_this();
    Timer::SPtr front_timer;
    bool at_front;
    if (!func_) {
        return false;
    }
    auto it = manager_->timers_.find(timer);
    if (it == manager_->timers_.end()) {
        return false;
    }
    manager_->timers_.erase(it);
    int64_t start = 0;
    if (from_now) {
        start = GetCurrentTime();
    } else {
        start = expire_ - interval_; //上一次超时时间
    }
    interval_ = interval;
    expire_ = start + interval_;
    manager_->timers_.insert(timer);
    //如果at_front为真，那么释放写锁后就会通知iomanager重新获取最近超时时间
    //need_notify_防止多个timer调用reset之后at_front都为真，然后同时通知
    //如前一个timer调用reset通知iomanager，后一个更近的timer也也用reset
    //但这两个onTimerInsertAtFront都会调用getExpire获取最近时间，实际做的是一样的动作
    //所以在调用getExpire前，应该只有一个timer调用reset之后能通知iomanager
    at_front = (timer == *manager_->timers_.begin()) && !manager_->need_notify_;
    if (at_front) {
        manager_->need_notify_ = true;
    }

    //timer更新为最近超时时间
    if (at_front) {
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
    bool at_front;
    Timer::SPtr timer(new Timer(interval, func, recursive, this));
    auto it = timers_.insert(timer).first;
    at_front = (it == timers_.begin());

    //比原来的超时时间更近，唤醒iomanager重新设置epoll_wait的超时时间
    if (at_front) {
        timerInsertAtFront();
    }
    return timer;
}

static void OnTimer(std::weak_ptr<void> weak_cond, std::function<void()> func) {
    // LOG_DEBUG << "on timer";
     std::shared_ptr<void> temp = weak_cond.lock();
     if (temp) {
         LOG_INFO << "on timer and invoke expire function";
         func();
     }
}

Timer::SPtr TimeManager::addConditonTimer(int64_t interval, 
                                          std::function<void()> func,
                                          std::weak_ptr<void> weak_cond,
                                          bool recursive) {
    return addTimer(interval, std::bind(&OnTimer, weak_cond, func), recursive);
}

int64_t TimeManager::getExpire() {
    need_notify_ = false;
    if (timers_.empty()) {
        return -1;
    }

    const Timer::SPtr& last_expire = *timers_.begin();
    int64_t now = GetCurrentTime();
    if (now > last_expire->expire_) {
        return 0;
    } else {
        return last_expire->expire_ - now;
    }
}

//TODO: 能不能在遍历时加入超时回调
void TimeManager::expiredFunctions(std::vector<std::function<void()> >& expired_funcs) {
    if (timers_.empty()) {
        return;
    }

    if (timers_.empty()){
        return;
    }

    int64_t now = GetCurrentTime();
    std::vector<Timer::SPtr> expired_timers;
    Timer::SPtr now_timer(new Timer(now));
    auto it = timers_.lower_bound(now_timer);

    while (it != timers_.end() && (*it)->expire_ == now) {
        ++it;
    }
    expired_timers.insert(expired_timers.begin(), timers_.begin(), it);
    timers_.erase(timers_.begin(), it);
    expired_funcs.reserve(expired_timers.size());

    for (auto& timer : expired_timers) {
        expired_funcs.push_back(timer->func_);
        if (timer->recursive_) {
            timer->expire_ = now + timer->interval_;
            timers_.insert(timer);
        } else {
            //防止再进入cancel函数清除timer
            timer->func_ = nullptr;
        }
    }
}

bool TimeManager::hasTimer() {
    return !timers_.empty();
}


} //namespace reyao
