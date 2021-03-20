#include "timestamp.h"

#include <string>
#include <sys/time.h>

namespace qlog {

namespace detail {

std::string TimeStamp::dateTime() const {
    // cache format time, flush per second.
    static thread_local time_t t_second = 0;
    static thread_local char t_datetime[32]; // 2021-03-11 23:47:22
    time_t now_sec = timestamp_ / kUsecPerSec;
    if (t_second < now_sec) {
        t_second = now_sec;
        struct tm st_time;
        localtime_r(&t_second, &st_time);
        strftime(t_datetime, sizeof(t_datetime), "%Y-%m-%d %H:%M:%S", &st_time);
    }

    return t_datetime;
}

std::string TimeStamp::format() const {
    char fmt[32];
    uint32_t usec = static_cast<uint32_t>(timestamp_ % kUsecPerSec);
    snprintf(fmt, sizeof(fmt), "%s.%06u", dateTime().c_str(), usec);
    return fmt;
}


} // namespace detail

} // namespace qlog