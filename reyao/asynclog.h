#pragma once

#include "reyao/fixedbuffer.h"
#include "reyao/thread.h"
#include "reyao/nocopyable.h"

#include <memory>
#include <vector>

namespace reyao {

static const int kBufferSize = 4096; //异步日志缓冲区大小

//日志滚动和写入文件类
class LogFile : public NoCopyable {
public:
    LogFile();

    void append(const char* logline, size_t len);
    void write(const char* logline, size_t len);
    void flush();

private:
    //NOTE: 如果1秒内写入超过64M，在RollFile时打开的是同一文件
    void rollFile();
    void getFileName(time_t& now, std::string& filename);

    static const time_t kRollPerSeconds = 24 * 60 * 60;     //一天的秒数
    static const long kFlushInterval = 3;                   //最快3秒一次将缓冲区刷新到内核中
    static const size_t kCheckPerN = 1024;                  //每记录1024条日志检查是否滚动日志以及冲洗缓冲区
    static const size_t kMaxFileSize = 64 * 1024 * 1024;    //每个log文件最大64M
    static const int kFileBufSize = 64 * 1024;              //文件IO缓冲区大小

    size_t count_ = 0;      //缓冲区的日志数
    size_t writed_ = 0;     //文件已写大小
    time_t last_flush_ = 0; //上一次刷新缓冲区时间
    time_t last_roll_ = 0;  //上一次滚动日志文件时间
    
    FILE* fp_ = nullptr;        //文件IO指针
    char fp_buf_[kFileBufSize]; //文件IO的缓冲区
};

//异步写日志类
class AsyncLog : public NoCopyable {
public:
    typedef FixedBuffer<kBufferSize> Buffer;
    typedef std::vector<std::unique_ptr<Buffer>> BufferVec;
    typedef BufferVec::value_type BufferUPtr;
    AsyncLog();
    ~AsyncLog();

    void append(const char* logline, size_t len);
    void start();
    void stop();
    bool isStart() { return running_;}

private:
    void threadFunc();

    bool running_;
    Mutex mutex_;
    Condition cond_;
    CountDownLatch latch_;
    Thread thread_;
    BufferUPtr curr_buffer_;
    BufferUPtr next_buffer_;
    BufferVec buffers_;
};

} //namespace reyao
