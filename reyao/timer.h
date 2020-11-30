#pragma once

#include "reyao/nocopyable.h"

#include <memory>
#include <functional>
#include <set>
#include <vector>

namespace reyao {

class TimeManager;

class Timer : public std::enable_shared_from_this<Timer>, 
              public NoCopyable {
friend class TimeManager;

public:
    typedef std::shared_ptr<Timer> SPtr;

    Timer(int64_t interval, std::function<void()> func, bool recursive,
          TimeManager* manager);
    explicit Timer(int64_t expire);

    //从TimerManager中取消定时事件
    bool cancel();
    //从当前时刻开始重新设置定时事件
    bool refresh();
    //设置间隔时间
    bool reset(int64_t interval, bool from_now);

private:
    TimeManager* manager_ = nullptr;
    bool recursive_ = false;            //是否循环触发定时器
    int64_t interval_ = 0;              //执行周期
    int64_t expire_ = 0;                //超时时间
    std::function<void()> func_;        //回调

    struct Comparator {
       bool operator()(const Timer::SPtr& lhs, const Timer::SPtr& rhs) const;
    };
};

class TimeManager {

friend class Timer;

public:
    TimeManager();
    virtual ~TimeManager();

    Timer::SPtr addTimer(int64_t interval, std::function<void()> func,
                      bool recursive = false);
    //传入一个weak_ptr条件，如果超时后weak_ptr取不到，则不执行回调
    Timer::SPtr addConditonTimer(int64_t interval, std::function<void()> func,
                                 std::weak_ptr<void> weak_cond, bool recursive = false);
    void expiredFunctions(std::vector<std::function<void()> >& expired_funcs);
    int64_t getExpire();

protected:
    //通知epoll_wait重新设置超时时间
    virtual void timerInsertAtFront() = 0;
    bool hasTimer();

private:

    std::set<Timer::SPtr, Timer::Comparator> timers_;
    bool need_notify_ = false;
};

} //namespace reyao