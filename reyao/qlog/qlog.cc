#include "qlog.h"
#include "format.h"

namespace qlog {

const char* LogLevelToString(LogLevel level) {
    switch (level) {
    case LogLevel::TRACE:
        return "TRACE";
    case LogLevel::DEBUG:
        return "DEBUG";
    case LogLevel::INFO:
        return "INFO";
    case LogLevel::ERROR:
        return "ERROR";
    case LogLevel::WARN:
        return "WARN";
    case LogLevel::FATAL:
        return "FATAL";
    }
    return "UNKNOWN";
}

uint32_t RingBuffer::read(char* to, uint32_t n) {
    uint32_t readable = std::min(canRead(), n);
    uint32_t offset2End = std::min(readable, kBufferSize - offsetOfPos(readPos_));
    memcpy(to, buf_ + offsetOfPos(readPos_), offset2End);
    memcpy(to + offset2End, buf_, readable - offset2End);

    readPos_ += readable;
    std::atomic_thread_fence(std::memory_order_release);

    return readable;
}

void RingBuffer::write(const char* from, uint32_t n) {
    while (getUnusedBytes() < n) {
    }
    assert(buf_);
    uint32_t offset2End = std::min(n, kBufferSize - offsetOfPos(writePos_));
    memcpy(buf_ + offsetOfPos(writePos_), from, offset2End);
    memcpy(buf_, from + offset2End, n - offset2End);
    writePos_ += n;
    std::atomic_thread_fence(std::memory_order_release);
}

QLog::QLog() 
    : level_(LogLevel::DEBUG),
      appender_(ConsoleAppender::GetInstance()),
      prepareExit_(false),
      threadExit_(false),
      outputBuf_(nullptr),
      doubleBuf_(nullptr),
      bufferSize_(64 * 1024 * 1024), // 64 M
      totalReadBytes_(0),
      perReadBytes_(0),
      threadBuffers_(),
      sinkThread_(),
      bufferMtx_(),
      condMtx_(),
      empty_(),
      process_() {
        
    outputBuf_ = static_cast<char *>(malloc(bufferSize_));
    doubleBuf_ = static_cast<char *>(malloc(bufferSize_));
    sinkThread_ = std::thread(&QLog::sinkThreadFunc, this);      
}

QLog::~QLog() {
    {
        // notify background log thread. Wait all
        // buffers' data sink to disk.
        std::unique_lock<std::mutex> lock(condMtx_);
        prepareExit_ = true;
        process_.notify_all();
        empty_.wait(lock);
    }

    {
        // now log thread already read all logs, exit.
        std::lock_guard<std::mutex> lock(condMtx_);
        threadExit_ = true;
        // log thread may find no data to sink and 
        // continue to slepp on process_ at tha last time.
        process_.notify_all();
    }

    if (sinkThread_.joinable()) {
        sinkThread_.join();
    }

    free(outputBuf_);
    free(doubleBuf_);

    for (auto& buf : threadBuffers_) {
        delete buf;
    }

}

void QLog::sinkThreadFunc() {
    while (!threadExit_) {

        // lock thread_buffers to avoid reading vector
        // while a new thread push back a new buffer.
        {
            std::lock_guard<std::mutex> lock(bufferMtx_);
            uint32_t buffer_idx = 0;
            while (!threadExit_ && !outputFull_ && 
                   (buffer_idx < threadBuffers_.size())) {
                RingBuffer *buf = threadBuffers_[buffer_idx];
                uint32_t readable = buf->canRead();

                if (bufferSize_ - perReadBytes_ < readable) {
                    outputFull_ = true;
                    break;
                }

                if (readable > 0) {
                    uint32_t n = buf->read(outputBuf_ + perReadBytes_, readable);
                    perReadBytes_ += n;
                }
                ++buffer_idx;
            }
        }

        // no data to sink, go to sleep.
        if (perReadBytes_ == 0) {
            std::unique_lock<std::mutex> lock(condMtx_);

            // no data, be notifyed that process is comming
            // exit. Get last chance to see if still have data
            // come.
            if (prepareExit_) {
                prepareExit_ = false;
                continue;
            }

            // static object QLog will be destructed when main thread
            // exit. However, it should ensure all logs have benn sink
            // to disk. So only if background log thread find buffers
            // already empty and notify main thread can main thread
            // destruct Log object.
            empty_.notify_one();
            process_.wait_for(lock, std::chrono::microseconds(50));
        } else {
            appender_->write(outputBuf_, perReadBytes_);

            totalReadBytes_ += perReadBytes_;
            perReadBytes_ = 0;
            outputFull_ = false;
        }
    }
}

RingBuffer* QLog::buffer() {
    static thread_local RingBuffer* buf = nullptr;
    if (!buf) {
        std::lock_guard<std::mutex> lock(bufferMtx_);
        buf = new RingBuffer;
        threadBuffers_.push_back(buf);
    }
    return buf;
}

} // namespace qlog