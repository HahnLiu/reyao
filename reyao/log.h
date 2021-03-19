#pragma once

#include "reyao/fixedbuffer.h"
#include "reyao/asynclog.h"
#include "reyao/singleton.h"
#include "reyao/mutex.h"
#include "reyao/coroutine.h"
#include "reyao/nocopyable.h"

#include <time.h>
#include <string.h>
#include <stdarg.h>

#include <list>
#include <vector>
#include <memory>
#include <string>
#include <sstream>
#include <map>


#define g_logger Singleton<reyao::Logger>::GetInstance()

#define LOG_LEVEL(level) \
    if (reyao::g_logger->getLevel() <= level) \
        reyao::LogDataWrap(level, time(nullptr), reyao::Thread::GetThreadId(), \
        reyao::Coroutine::GetCoroutineId(), reyao::Thread::GetThreadName(),  \
        __FILENAME__, __LINE__).getDataStream()

#define LOG_DEBUG  LOG_LEVEL(reyao::LogLevel::DEBUG)
#define LOG_INFO  LOG_LEVEL(reyao::LogLevel::INFO)
#define LOG_WARN  LOG_LEVEL(reyao::LogLevel::WARN)
#define LOG_ERROR  LOG_LEVEL(reyao::LogLevel::ERROR)
#define LOG_FATAL  LOG_LEVEL(reyao::LogLevel::FATAL)

#define LOG_FMT(level, fmt, ...) \
    if (reyao::g_logger->getLevel() <= level)  \
        reyao::LogDataWrap(level, time(nullptr), reyao::Thread::GetThreadId(), \
        reyao::Coroutine::GetCoroutineId(), reyao::Thread::GetThreadName(),  \
        __FILENAME__, __LINE__).getData().format(fmt, ## __VA_ARGS__)

#define LOG_FMT_DEBUG(fmt, ...) LOG_FMT(reyao::LogLevel::DEBUG, fmt, ## __VA_ARGS__)
#define LOG_FMT_INFO(fmt, ...) LOG_FMT(reyao::LogLevel::INFO, fmt, ## __VA_ARGS__)
#define LOG_FMT_WARN(fmt, ...) LOG_FMT(reyao::LogLevel::WARN, fmt, ## __VA_ARGS__)
#define LOG_FMT_ERROR(fmt, ...) LOG_FMT(reyao::LogLevel::ERROR, fmt, ## __VA_ARGS__)
#define LOG_FMT_FATAL(fmt, ...) LOG_FMT(reyao::LogLevel::FATAL, fmt, ## __VA_ARGS__)

#define __FILENAME__ strrchr(__FILE__, '/') ? (strrchr(__FILE__, '/') + 1) : __FILE__

namespace reyao {

class Logger;

//日志级别
class LogLevel {
public:
    enum Level {   
        DEBUG = 1,
        INFO = 2,
        WARN = 3,
        ERROR = 4,
        FATAL = 5
    };

    static const char* ToString(LogLevel::Level level);
};

//日志消息类
class LogData : public NoCopyable {
public:
    LogData(LogLevel::Level level, time_t time,
            uint32_t threadId, uint32_t coroutineId, 
            std::string threadName, const char* fileName, uint32_t line);

    LogLevel::Level getLevel() const { return level_; }
    std::string getContent() const { return content_.str(); }
    std::stringstream& getContentStream() { return content_; }
    time_t getTime() const { return time_; }
    uint32_t getThreadId() const { return threadId_; }
    uint32_t getCoroutineId() const { return coroutineId_; }
    const std::string& getThreadName() const { return threadName_; }
    const char* getFileName() const { return fileName_; }
    uint32_t getLine() const { return line_; }

    void format(const char* fmt, ...);
    void format(const char* fmt, va_list al);

private:
    LogLevel::Level level_;  
    std::stringstream content_;  
    time_t time_;                       
    uint32_t threadId_;        
    uint32_t coroutineId_;     
    std::string threadName_;         
    const char* fileName_ = nullptr;   
    uint32_t line_ = 0;           
};

class LogDataWrap : public NoCopyable {
public:
    LogDataWrap(LogLevel::Level level, time_t time,
                uint32_t threadId, uint32_t coroutineId, 
                std::string threadName, const char* fileName, uint32_t line);
    ~LogDataWrap();

    std::stringstream& getDataStream() { return data_.getContentStream(); }
    LogData& getData() { return data_; }

private:
    LogData data_;
};

class LogFormatter : public NoCopyable {   
public:
    typedef std::shared_ptr<LogFormatter> SPtr;
    class FmtItem { 
    public:
        typedef std::shared_ptr<FmtItem> SPtr;
        explicit FmtItem(const std::string& str = "") {}
        virtual ~FmtItem() {}
        virtual void format(std::stringstream& ss,const LogData& data) = 0;
    };

public:
    explicit LogFormatter(const std::string& pattern);

    void init();
    std::string format(const LogData& data);

private:
    std::string pattern_;
    std::vector<FmtItem::SPtr> items_;
};

class LogAppender : public NoCopyable {
public:
    typedef std::shared_ptr<LogAppender> SPtr;
    LogAppender() {}
    virtual ~LogAppender() {}

    virtual void append(const std::string& logline) = 0;
};

class ConsoleAppender : public LogAppender {
public:
    void append(const std::string& logline) override;
};

class FileAppender : public LogAppender {
public:
    FileAppender();
    ~FileAppender();

    void append(const std::string& logline) override;

private:
    AsyncLog log_;
};


class Logger : public NoCopyable {
public:
    Logger();

    void append(const LogData& data);
    void setLevel(LogLevel::Level level);
    LogLevel::Level getLevel() const { return level_; }

    void setFormatter(std::string pattern);
    void setConsoleAppender();
    void setFileAppender();

private:
    LogLevel::Level level_;        
    LogFormatter::SPtr formatter_; 
    LogAppender::SPtr appender_;   
    Mutex mutex_;
};

} // namespace reyao
