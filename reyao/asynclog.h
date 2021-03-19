#pragma once

#include "reyao/fixedbuffer.h"
#include "reyao/thread.h"
#include "reyao/nocopyable.h"

#include <memory>
#include <vector>

namespace reyao {

static const int kBufferSize = 4096; 

class LogFile : public NoCopyable {
public:
    LogFile();

    void append(const char* logline, size_t len);
    void write(const char* logline, size_t len);
    void flush();

private:
    void rollFile();
    void getFileName(time_t& now, std::string& filename);

    static const time_t kRollPerSeconds = 24 * 60 * 60;  
    static const long kFlushInterval = 3;               
    static const size_t kCheckRollPerN = 1024;           
    static const size_t kMaxFileSize = 64 * 1024 * 1024;
    static const int kFileBufSize = 64 * 1024; 

    size_t count_ = 0;    
    size_t writed_ = 0;    
    time_t lastFlush_ = 0; 
    time_t lastRoll_ = 0;  
    
    FILE* fp_ = nullptr;   
    char fp_buf_[kFileBufSize]; 
};

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
    bool isStart() { return running_; }

private:
    void threadFunc();

    bool running_;
    bool exit_{false};
    Mutex mutex_;
    Condition cond_;
    CountDownLatch latch_;
    Thread thread_;
    BufferUPtr currBuffer_;
    BufferUPtr backupBuffer_;
    BufferVec buffers_;
};

} // namespace reyao
