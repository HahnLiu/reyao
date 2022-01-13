#include "reyao/log.h"
#include "reyao/util.h"

#include <iostream>
#include <functional>

namespace reyao {

thread_local char t_time[64]; 
thread_local time_t t_last_second;

static const char* s_defaultLogFormat = "[%p %d]%T[%t %N %c]%T%f:%l %T%m%n";

LogData::LogData(LogLevel::Level level, time_t time,
                 uint32_t threadId, uint32_t coroutineId, 
                 std::string threadName, const char *fileName, uint32_t line)
    : level_(level),
      time_(time),
      threadId_(threadId),
      coroutineId_(coroutineId),
      threadName_(threadName),
      fileName_(fileName),
      line_(line) {
                     
}

void LogData::format(const char* fmt, ...) {
    va_list al;
    va_start(al, fmt);
    format(fmt, al);
    va_end(al);
}

void LogData::format(const char* fmt, va_list al) {
    char* buf = nullptr;
    int len = vasprintf(&buf, fmt, al);
    if(len != -1) {
        content_ << std::string(buf, len);
        free(buf);
    }
}
        
LogDataWrap::LogDataWrap(LogLevel::Level level, time_t time,
            uint32_t threadId, uint32_t coroutineId, std::string threadName, 
            const char* fileName, uint32_t line)
    : data_(level, time, threadId, coroutineId, 
            threadName, fileName, line) {

}

LogDataWrap::~LogDataWrap() {
    g_logger->append(data_);
}

class MessageFmtItem : public LogFormatter::FmtItem {
 public:
    explicit MessageFmtItem(const std::string &str = "") {}
    void format(std::stringstream &ss, const LogData& data) override {
        ss << data.getContent();
    }
};

class LevelFmtItem : public LogFormatter::FmtItem {
 public:
    explicit LevelFmtItem(const std::string& str = "") {}
    void format(std::stringstream& ss, const LogData& data) override { 
        ss << LogLevel::ToString(data.getLevel());
    }
};

class DateTimeFmtItem : public LogFormatter::FmtItem {
 public:
    explicit DateTimeFmtItem(const std::string& str = "") {}
    void format(std::stringstream& ss, const LogData& data) override {   
        struct tm tm;
        time_t second = data.getTime();
        if (second != t_last_second)  {
            t_last_second = second;
            localtime_r(&second, &tm);
            strftime(t_time, sizeof(t_time), "%Y-%m-%d %H:%M:%S", &tm);
        }
        ss << t_time;
    }
};

class ThreadIdFmtItem : public LogFormatter::FmtItem {
 public:
    explicit ThreadIdFmtItem(const std::string& str = "") {}
        void format(std::stringstream& ss, const LogData& data) override {
        ss << data.getThreadId();
    }
};

class CoroutineIdFmtItem : public LogFormatter::FmtItem {
 public:
    explicit CoroutineIdFmtItem(const std::string& str = "") {}
    void format(std::stringstream& ss, const LogData& data) override {
        ss << data.getCoroutineId();
    }
};

class ThreadNameFmtItem : public LogFormatter::FmtItem {
 public:
    explicit ThreadNameFmtItem(const std::string& str = "") {}
    void format(std::stringstream& ss, const LogData& data) override {
        ss << data.getThreadName();
    }
};

class FileNameFmtItem : public LogFormatter::FmtItem {
 public:
    explicit FileNameFmtItem(const std::string& str = "") {}
    void format(std::stringstream& ss, const LogData& data) override {
        ss << data.getFileName();
    }
};

class LineFmtItem : public LogFormatter::FmtItem {
 public:
    explicit LineFmtItem(const std::string& str = "") {}
    void format(std::stringstream& ss, const LogData& data) override {
        ss << data.getLine();
    }
};

class NextLineFmtItem : public LogFormatter::FmtItem {
 public:
    explicit NextLineFmtItem(const std::string& str = "") {}
    void format(std::stringstream& ss, const LogData& data) override {
        ss << "\n";
    }
};

class TableFmtItem : public LogFormatter::FmtItem {
 public:
    explicit TableFmtItem(const std::string& str = "") {}
    void format(std::stringstream& ss, const LogData& data) override {
        ss << "\t";
    }
};

class StringFmtItem : public LogFormatter::FmtItem {
 public:
    explicit StringFmtItem(const std::string& str):str_(str) {}
    void format(std::stringstream& ss, const LogData& data) override {
        ss << str_;
    }
 private:
    std::string str_;
};

const char* LogLevel::ToString(LogLevel::Level level) {
    switch (level) {
        case LogLevel::DEBUG:   return "DEBUG";
        case LogLevel::INFO:    return "INFO";
        case LogLevel::WARN:    return "WARN";
        case LogLevel::ERROR:   return "ERROR";
        case LogLevel::FATAL:   return "FATAL";

        default:    return "UNKNOW";
    }
}

LogFormatter::LogFormatter(const std::string& pattern): pattern_(pattern) {
    init();
}

void LogFormatter::init() {   
    //str type  // %str
    std::vector<std::pair<std::string, int>> vec;
    std::string s;
    for (size_t i = 0; i < pattern_.size(); i++) {
        if (pattern_[i] != '%') {
            s.append(1, pattern_[i]);
            continue;
        }
    

        if (i + 1 < pattern_.size() && pattern_[i + 1] == '%') {
            s.append(1, '%');
            continue;
        }

        size_t n = i + 1;
        std::string str;
        while (n < pattern_.size()) {
            if (!isalpha(pattern_[n])) {
                str = pattern_.substr(i + 1, n - i - 1);
                break;
            }       
            ++n;
            if (n == pattern_.size()) {
                if (str.empty()) {
                    str = pattern_.substr(i + 1);
                }
            }
        }

        if (!s.empty()) {
            vec.push_back({s, 0});
            s.clear();
        }
        vec.push_back({str, 1});
        i = n - 1;
    }

    if (!s.empty()) {
        vec.push_back({s, 0});
    }

    static std::map<std::string, std::function<FmtItem::SPtr()>> format2Item = {
#define XX(c, Item) \
        {#c, [] () { return FmtItem::SPtr(std::make_shared<Item>());} }

        XX(m, MessageFmtItem),
        XX(p, LevelFmtItem),
        XX(d, DateTimeFmtItem),
        XX(t, ThreadIdFmtItem),
        XX(c, CoroutineIdFmtItem),
        XX(N, ThreadNameFmtItem),
        XX(f, FileNameFmtItem),
        XX(l, LineFmtItem),
        XX(n, NextLineFmtItem),
        XX(T, TableFmtItem),

#undef XX
    };

    for (auto& i : vec) {
        if (i.second == 0) {
            items_.push_back(FmtItem::SPtr(std::make_shared<StringFmtItem>(i.first))); 
        } else {
            auto iter = format2Item.find(i.first);
            if (iter == format2Item.end()) {
                items_.push_back(FmtItem::SPtr(std::make_shared<StringFmtItem>("<<error format %" + i.first + ">>"))); //格式字符不在map里
            } else {
                items_.push_back(iter->second()); 
            }
        }
    }
}

std::string LogFormatter::format(const LogData& data) {
    std::stringstream ss;
    for (auto item : items_) {
        item->format(ss, data);
    }
    return ss.str();
}

Logger::Logger()
    : level_(LogLevel::DEBUG),
      formatter_(std::make_shared<LogFormatter>(s_defaultLogFormat)), 
      appender_(std::make_shared<FileAppender>()) {

}

void Logger::setLevel(LogLevel::Level level) {   
    MutexGuard lock(mutex_);
    level_ = level; 
}

void Logger::setFormatter(std::string pattern) {   
    MutexGuard lock(mutex_);
    formatter_ = std::make_shared<LogFormatter>(pattern);
}

void Logger::setConsoleAppender() {   
    MutexGuard lock(mutex_);
    appender_ = std::make_shared<ConsoleAppender>();
}

void Logger::setFileAppender() {
    MutexGuard lock(mutex_);
    appender_ = std::make_shared<FileAppender>();
}

void Logger::append(const LogData& data) {   
    std::shared_ptr<LogFormatter> formatter;
    std::shared_ptr<LogAppender> appender;
    {
        MutexGuard lock(mutex_);
        formatter = formatter_;
        appender = appender_;
    }
    std::string logline = formatter->format(data);
    appender->append(logline);
}

void ConsoleAppender::append(const std::string& logline) {
    printf(logline.c_str(), logline.size());
}

FileAppender::FileAppender() {
    log_.start();
}

FileAppender::~FileAppender() {   
    log_.stop();
}

void FileAppender::append(const std::string& logline) {
    log_.append(logline.c_str(), logline.size());
}

} // namespace reyao
