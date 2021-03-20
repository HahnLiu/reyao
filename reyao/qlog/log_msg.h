#pragma once
#include "qlog.h"

namespace qlog {

class LogMsg {
public:
    LogMsg(LogLevel level, const char* file, 
           const char* function, uint32_t line);
    ~LogMsg();

public:
    LogLevel getLevel() const { return level_; }
    const char* getFile() const { return file_; }
    const char* getFunction() const { return function_; }
    uint32_t getLine() const { return line_; }

public:
    LogMsg &operator<<(bool arg) {
        if (arg)
            append("true", 4);
        else
            append("false", 5);
        return *this;
    }

    LogMsg &operator<<(char arg) {
        append(&arg, 1);
        return *this;
    }

    LogMsg &operator<<(int16_t arg) {
        char tmp[8];
        size_t len = detail::i16toa(arg, tmp);
        append(tmp, len);
        return *this;
    }

    LogMsg &operator<<(uint16_t arg) {
        char tmp[8];
        size_t len = detail::u16toa(arg, tmp);
        append(tmp, len);
        return *this;
    }

    LogMsg &operator<<(int32_t arg) {
        char tmp[12];
        size_t len = detail::i32toa(arg, tmp);
        append(tmp, len);
        return *this;
    }

    LogMsg &operator<<(uint32_t arg) {
        char tmp[12];
        size_t len = detail::u32toa(arg, tmp);
        append(tmp, len);
        return *this;
    }

    LogMsg &operator<<(int64_t arg) {
        char tmp[24];
        size_t len = detail::i64toa(arg, tmp);
        append(tmp, len);
        return *this;
    }

    LogMsg &operator<<(uint64_t arg) {
        char tmp[24];
        size_t len = detail::u64toa(arg, tmp);
        append(tmp, len);
        return *this;
    }

    LogMsg &operator<<(double arg) {
        append(std::to_string(arg).c_str());
        return *this;
    }

    LogMsg &operator<<(const char *arg) {
        append(arg);
        return *this;
    }

    LogMsg &operator<<(const std::string &arg) {
        append(arg.c_str(), arg.length());
        return *this;
    }

private:
    void append(const char* data, size_t n);
    void append(const char* data) {
        assert(data != nullptr);
        append(data, strlen(data));
    }

private:
    uint32_t count_; // log data bytes.
    LogLevel level_;
    const char* file_;
    const char* function_;
    uint32_t line_;
};

} // namespace qlog