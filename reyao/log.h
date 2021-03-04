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

//创建临时LogDataWrap对象，返回一个流，LogDataWrap对象析构时append到Logger中
#define LOG_LEVEL(level) \
    if (reyao::g_logger->getLevel() <= level) \
        reyao::LogDataWrap(level, time(nullptr), 0, reyao::Thread::GetThreadId(), \
        reyao::Coroutine::GetCoroutineId(), reyao::Thread::GetThreadName(),  \
        __FILENAME__, __LINE__).getDataStream()

#define LOG_DEBUG  LOG_LEVEL(reyao::LogLevel::DEBUG)
#define LOG_INFO  LOG_LEVEL(reyao::LogLevel::INFO)
#define LOG_WARN  LOG_LEVEL(reyao::LogLevel::WARN)
#define LOG_ERROR  LOG_LEVEL(reyao::LogLevel::ERROR)
#define LOG_FATAL  LOG_LEVEL(reyao::LogLevel::FATAL)

#define LOG_FMT(level, fmt, ...) \
    if (reyao::g_logger->getLevel() <= level)  \
        reyao::LogDataWrap(level, time(nullptr), 0, reyao::Thread::GetThreadId(), \
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
    LogData(LogLevel::Level level, time_t time, uint32_t elapse,
            uint32_t thread_id, uint32_t coroutine_id, 
            std::string thread_name, const char* file_name, uint32_t line);

    LogLevel::Level getLevel() const { return level_; }
    std::string getContent() const { return content_.str(); }
    std::stringstream& getContentStream() { return content_; }
    time_t getTime() const { return time_; }
    uint32_t getElapse() const { return elapse_; }
    uint32_t getThreadId() const { return thread_id_; }
    uint32_t getCoroutineId() const { return coroutine_id_; }
    const std::string& getThreadName() const { return thread_name_; }
    const char* getFileName() const { return file_name_; }
    uint32_t getLine() const { return line_; }

    void format(const char* fmt, ...);
    void format(const char* fmt, va_list al);

private:
    LogLevel::Level level_;             //日志等级
    std::stringstream content_;         //日志内容
    time_t time_;                       //时间戳
    uint32_t elapse_;                   //程序启动至今所用毫秒数
    uint32_t thread_id_;                //线程id
    uint32_t coroutine_id_;             //协程id
    std::string thread_name_;           //线程名称
    const char* file_name_ = nullptr;   //文件名
    uint32_t line_ = 0;                  //行号
};

class LogDataWrap : public NoCopyable {
public:
    LogDataWrap(LogLevel::Level level, time_t time, uint32_t elapse,
                uint32_t thread_id, uint32_t coroutine_id, 
                std::string thread_name, const char* file_name, uint32_t line);
    ~LogDataWrap();

    std::stringstream& getDataStream() { return data_.getContentStream(); }
    LogData& getData() { return data_; }

private:
    LogData data_;
};

//日志格式类
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

//日志输出虚基类
class LogAppender : public NoCopyable {
public:
    typedef std::shared_ptr<LogAppender> SPtr;
    LogAppender() {}
    virtual ~LogAppender() {}

    virtual void append(const std::string& logline) = 0;
};

//stdout输出类
class ConsoleAppender : public LogAppender {
public:
    void append(const std::string& logline) override;
};

//文件输出类
class FileAppender : public LogAppender {
public:
    FileAppender();
    ~FileAppender();

    void append(const std::string& logline) override;

private:
    AsyncLog log_;
};

//日志类
class Logger : public NoCopyable {
public:
    Logger();

    void append(const LogData& data);
    void setLevel(LogLevel::Level level);
    LogLevel::Level getLevel() const { return level_; }
    //格式模板
    //%m --- 消息体
    //%p --- 优先级
    //%d --- 时间戳
    //%r --- 启动后时间
    //%t --- 线程id
    //%c --- 协程id
    //%N --- 线程名称
    //%f --- 文件名
    //%l --- 行号
    //%n --- 回车换行
    //%T --- 制表符
    void setFormatter(std::string pattern);
    void setConsoleAppender();
    void setFileAppender();

private:
    LogLevel::Level level_;                     //日志级别
    LogFormatter::SPtr formatter_;   //日志格式
    LogAppender::SPtr appender_;     //日志的输出目的地
    Mutex mutex_;
};

} //namespace reyao
