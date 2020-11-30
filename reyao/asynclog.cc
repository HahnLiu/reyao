#include "reyao/asynclog.h"

namespace reyao {

LogFile::LogFile(): count_(0),writed_(0) {
    rollFile();
}

//往文件IO写并负责滚动日志文件和刷新缓冲区
void LogFile::append(const char* logline, size_t len) {   
    //超过文件大小则滚动日志
    if (writed_ + len > kMaxFileSize) {
        rollFile();
    }

    write(logline, len);
    writed_ += len;
    count_ += 1;

    if (count_ >= kCheckPerN) {
        time_t now = time(NULL);
        //kRollPerSeconds是1天的秒数，除以kRollPerSeconds再乘以kRollPerSeconds得到当天0点对应的time_t
        time_t day = now / kRollPerSeconds * kRollPerSeconds;
        if (day > last_roll_) {
            rollFile(); //day为0点的时间，如果day大于上次滚动日志的时间，则到了新的一天，滚动日志
        }
        else if (now - last_flush_ > kFlushInterval) {
            last_flush_ = now;
            flush();
        }

        count_ = 0;
    }
}

//往文件IO写（先写进缓冲区）
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

//滚动日志文件
void LogFile::rollFile() {   
    if (fp_) {
        fclose(fp_);
    }
    time_t now = time(NULL);
    std::string filename;
    getFileName(now, filename);
    fp_ = fopen(filename.c_str(), "a");
    setbuffer(fp_, fp_buf_, kFileBufSize);
    last_flush_ = now;
    last_roll_ = now;
    count_ = 0;
    writed_ = 0;
}

//格式化文件名
void LogFile::getFileName(time_t& now, std::string& filename) {
    struct tm* str_time = localtime(&now);
    char buf[20];
    strftime(buf, sizeof(buf), "%Y%m%d-%H%M%S.log", str_time);
    buf[19] = '\0'; //char[]转string要用'\0'否则string溢出
    filename = buf;
}

AsyncLog::AsyncLog():
running_(false),
mutex_(),
cond_(mutex_),
latch_(1),
thread_(std::bind(&AsyncLog::threadFunc, this), "LogFileThread"),
curr_buffer_(new Buffer),
next_buffer_(new Buffer)
{
    buffers_.reserve(16); //避免频繁扩容
}

AsyncLog::~AsyncLog()
{
    if (running_)
    {
        stop();
    }
}

//前端写入缓冲区，添加日志信息不超过kBufferSize（4096）
void AsyncLog::append(const char* logline, size_t len)
{   
    MutexGuard lock(mutex_);
    if (curr_buffer_->avail() > len)
    {   
        curr_buffer_->append(logline, len);
        // std::cout << "add " << len << " bytes" <<std::endl;
    }
    else
    {
        buffers_.push_back(std::move(curr_buffer_));
        if (next_buffer_)
        {
            curr_buffer_ = std::move(next_buffer_);
        }
        else
        {
            curr_buffer_.reset(new Buffer);
        }
        curr_buffer_->append(logline, len);
        // std::cout << "add " << len << " bytes" <<std::endl;
        cond_.notify();
    }
}


//后端统一将缓冲区数据写入磁盘
//FIXME: 用CountDownLatch保证后端线程初始化完成后
//主线程从start返回，这时可以append添加数据，但如果
//append添加完立刻退出程序，可能还没切换到后端线程
//程序就终止了，append的数据还没写到磁盘就丢失了
void AsyncLog::threadFunc()
{
    BufferUPtr first_buffer(new Buffer);
    BufferUPtr second_buffer(new Buffer);
    BufferVec buffers;
    buffers.reserve(16);
    LogFile logfile;
    latch_.countDown(); //缓冲区和文件IO工具初始化完成，通知主线程可以开始添加数据
    while (running_)
    {
        {   
            MutexGuard lock(mutex_);
            if (buffers_.empty())
            {
                cond_.waitForSeconds(2); //等待直到超时或前端添加数据后notify
            }
            buffers_.push_back(std::move(curr_buffer_));
            curr_buffer_ = std::move(first_buffer);
            if (!next_buffer_)
            {
                next_buffer_ = std::move(second_buffer);
            }

            buffers.swap(buffers_);
        } //end of cirtical section

        for (auto& buf : buffers)
            logfile.append(buf->data(), buf->size());

        first_buffer = std::move(buffers[0]);
        first_buffer->reset();

        //FIXME: 可以改进
        if (!second_buffer)
        {
            if (buffers.size() >= 2)
            {
                second_buffer = std::move(buffers[1]);
                second_buffer->reset();
            }
            else
            {   //前端curr_buffer_不够用时先拿next_buffer_,
                //再分配新的buffer，所以如果second_buffer为空
                //说明next_buffer_给curr_buffer_用了
                //那么buffers最少有两块buffer
                second_buffer.reset(new Buffer);
            }
        }
        //改进
        // second_buffer = std::move(buffers[1]);
        // second_buffer->Reset();

        buffers.clear();
        logfile.flush();
    } //end of running
}

void AsyncLog::start()
{
    if (!running_)
    {
        running_ = true;
        thread_.start();
        latch_.wait();
    }
}

void AsyncLog::stop()
{
    running_ = false;
    cond_.notify();
    thread_.join();
}

} // namespace reyao
