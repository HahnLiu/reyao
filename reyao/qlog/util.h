#pragma once

#include <sys/syscall.h>
#include <unistd.h>
#include <cstdint>
#include <string>

namespace qlog {

namespace detail {

inline pid_t Gettid() {
    static const thread_local pid_t t_thread_id = syscall(SYS_gettid);
    return t_thread_id;
}

std::string GetProcessName();

size_t u2a(uint64_t number, char *to);
size_t i2a(int64_t number, char *to);

size_t u16toa(uint16_t n, char *to);
size_t u32toa(uint32_t n, char *to);
size_t u64toa(uint64_t n, char *to);

size_t i16toa(int16_t n, char *to);
size_t i32toa(int32_t n, char *to);
size_t i64toa(int64_t n, char *to);


} // namespace detail

} // namespace qlog