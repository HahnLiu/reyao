#include "reyao/asynclog.h"

namespace reyao {

LogFile::LogFile(): count_(0),writed_(0) {
    rollFile();
}


void LogFile::append(const char* logline, size_t len) {   

    if (writed_ + len > kMaxFileSize) {
        rollFile();
    }

    write(logline, len);
    writed_ += len;
    count_ += 1;

    if (count_ >= kCheckRollPerN) {
        time_t now = time(NULL);
        time_t day = now / kRollPerSeconds * kRollPerSeconds;
        if (day > lastRoll_) {
            rollFile(); 
        }
        else if (now - lastFlush_ > kFlushInterval) {
            lastFlush_ = now;
            flush();
        }
        count_ = 0;
    }
}

void LogFile::write(const char* logline, size_t len) {
    size_t writed = 0;
    size_t left = len;
    while (left) {   
        size_t n = fwrite_unlocked(logline + writed, 1, left, fp_);
        if (n == 0) {
            int err = ferror(fp_);
            if (err) {
                fprintf(stderr, "WriteFile::append() fail\n");
            }
            break;
        }
        writed += n;
        left -= n;
    }
}

void LogFile::flush() { 
    fflush(fp_);
}

void LogFile::rollFile() {   
    if (fp_) {
        fclose(fp_);
    }
    time_t now = time(NULL);
    std::string filename;
    getFileName(now, filename);
    fp_ = fopen(filename.c_str(), "a");
    setbuffer(fp_, fp_buf_, kFileBufSize);
    lastFlush_ = now;
    lastRoll_ = now;
    count_ = 0;
    writed_ = 0;
}

//格式化文件名
void LogFile::getFileName(time_t& now, std::string& filename) {
    struct tm* str_time = localtime(&now);
    char buf[20];
    strftime(buf, sizeof(buf), "%Y%m%d-%H%M%S.log", str_time);
    buf[19] = '\0'; 
    filename = buf;
}

AsyncLog::AsyncLog()
    : running_(false),
      mutex_(),
      cond_(mutex_),
      latch_(1),
      thread_(std::bind(&AsyncLog::threadFunc, this), "LogFileThread"),
      currBuffer_(new Buffer),
      backupBuffer_(new Buffer) {
    buffers_.reserve(16);
}

AsyncLog::~AsyncLog() {
    if (running_) {
        stop();
    }
}

// max msg len: kBufferSize=4096
void AsyncLog::append(const char* logline, size_t len)
{   
    MutexGuard lock(mutex_);
    if (currBuffer_->avail() > len) {   
        currBuffer_->append(logline, len);
    }
    else {
        // 第一块缓冲区写完了，换第二块备用的，如果还不够则重新分配
        buffers_.push_back(std::move(currBuffer_));
        if (backupBuffer_) {
            currBuffer_ = std::move(backupBuffer_);
        }
        else {
            currBuffer_.reset(new Buffer);
        }
        currBuffer_->append(logline, len);
        // 有一块写完的缓冲区通知后端处理
        cond_.notify();
    }
}

void AsyncLog::threadFunc()
{
    BufferUPtr firstBuffer(new Buffer);
    BufferUPtr secondBuffer(new Buffer);
    BufferVec buffers;
    buffers.reserve(16);
    LogFile logfile;
    latch_.countDown();
    while (running_) {
        if (exit_) {
            // 收到前端退出信号，设置运行状态，最后一次写日志并退出
            running_ = false;
        }
        {   
            MutexGuard lock(mutex_);
            if (buffers_.empty()) {
                cond_.waitForSeconds(2); // 等待直到超时或前端写满缓冲区后通知
            }
            // 收集前端的缓冲区并分配新的缓冲区给前端
            buffers_.push_back(std::move(currBuffer_));
            currBuffer_ = std::move(firstBuffer);
            if (!backupBuffer_) {
                backupBuffer_ = std::move(secondBuffer);
            }

            buffers.swap(buffers_);
        } // end of cirtical section

        for (auto& buf : buffers)
            logfile.append(buf->data(), buf->size());

        // 复用刚写入磁盘的缓冲区，用于下次分配给前端
        firstBuffer = std::move(buffers[0]);
        firstBuffer->reset();
        if (!secondBuffer) {
            if (buffers.size() >= 2) {
                secondBuffer = std::move(buffers[1]);
                secondBuffer->reset();
            } else {   
                secondBuffer.reset(new Buffer);
            }
        }

        buffers.clear();
        logfile.flush();
    } // end of running
}

void AsyncLog::start()
{
    if (!running_) {
        running_ = true;
        thread_.start();
        latch_.wait();
    }
}

void AsyncLog::stop() {
    exit_ = true;
    cond_.notify();
    thread_.detach();
}

} // namespace reyao
