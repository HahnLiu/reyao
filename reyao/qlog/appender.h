#pragma once

#include <memory>
#include <string>
#include <unistd.h>
#include <iostream>

#include "log_file.h"
#include "util.h"

namespace qlog {


class LogAppender {
public:
    typedef std::shared_ptr<LogAppender> SPtr;
    LogAppender() {}
    virtual ~LogAppender() {}

    virtual void write(char* buf, uint32_t bytes) = 0;
};


class ConsoleAppender : public LogAppender {
public:
    static ConsoleAppender* GetInstance() {
        static ConsoleAppender appender;
        return &appender;
    }
    void write(char* buf, uint32_t bytes) override {
        fwrite(buf, bytes, 1, stdout);
    }
};

// 文件输出类
class FileAppender : public LogAppender {
public:
    static FileAppender* GetInstance() {
        static FileAppender appender;
        return &appender;
    }

private:
    FileAppender(std::string fileName = detail::GetProcessName(), 
                 size_t maxFileSize = kMaxFileSize)
        : logFile_(fileName, maxFileSize) {
        }
    ~FileAppender() {}

public:
    void write(char* buf, uint32_t bytes) override {
        logFile_.write(buf, bytes);
    }

    void setFileName(const std::string& name) { logFile_.setFileName(name); }
    void setMaxFileSize(size_t millBytes) { logFile_.setMaxFileSize(millBytes); }

private:
    static const size_t kMaxFileSize = 64 * 1024 * 1024;
    LogFile logFile_;
};

} // namespace qlog