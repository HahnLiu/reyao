#pragma once

#include <cstdint>
#include <string>
#include <sys/time.h> // gettimeofday

namespace qlog {

namespace detail {

class TimeStamp {
public:
    TimeStamp(): timestamp_(0) {}
    TimeStamp(uint64_t timestamp): timestamp_(timestamp) {}

    static TimeStamp Now() {
        struct timeval tv;
        gettimeofday(&tv, nullptr);
        uint64_t timestamp = tv.tv_sec * kUsecPerSec + tv.tv_usec;
        return TimeStamp(timestamp);
    }

    std::string dateTime() const;

    std::string format() const;

    uint64_t getUsec() const { return timestamp_; }
private:
    static const uint32_t kUsecPerSec = 1000 * 1000;
    uint64_t timestamp_;
};


} // namespace detail

} // namespace qlog