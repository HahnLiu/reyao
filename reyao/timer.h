#pragma once

#include "reyao/nocopyable.h"
#include "reyao/mutex.h"

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

    bool cancel();
    bool refresh();
    bool reset(int64_t interval, bool from_now);

private:
    TimeManager* manager_ = nullptr;
    bool recursive_ = false;        
    int64_t interval_ = 0;       
    int64_t expire_ = 0;          
    std::function<void()> func_;   

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
    Timer::SPtr addConditonTimer(int64_t interval, std::function<void()> func,
                                 std::weak_ptr<void> weakCond, bool recursive = false);

    void expiredFunctions(std::vector<std::function<void()> >& expired_funcs);
    int64_t getExpire();

protected:
    // notify epoller
    virtual void timerInsertAtFront() = 0;
    bool hasTimer();

private:
    std::set<Timer::SPtr, Timer::Comparator> timers_;
    bool needNotify_ = false;

    Mutex mutex_;
};

} // namespace reyao