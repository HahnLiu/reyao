#pragma once

#include <unistd.h>
#include <string>
#include <cstring> // strerror
#include <iostream>
#include <algorithm>

#include "timestamp.h"

namespace qlog {

class File {
public:
    File(): fp_(nullptr){}
    ~File() {
        if (fp_) {
            fclose(fp_);
        }
    }

    void open(const std::string& fileName, bool truncate = false) {
        fileName_ = fileName;
        const char* mode = truncate ? "w+" : "a+";
        fp_ = fopen(fileName_.c_str(), mode);
        if (!fp_) {
            std::cerr << "open " << fileName_ << " error: " << strerror(errno) << std::endl;
            exit(-1);
        }
    }

    void reopen(bool truncate = false) {
        close();
        open(fileName_, truncate);
    }

    void close() {
        if (fp_) {
            fclose(fp_);
            fp_ = nullptr;
        }
    }

    void fflush() {
        ::fflush(fp_);
    }

    void write(const char* data, size_t len) {
        if (!fp_) {
            open(fileName_);
        }
        if (fwrite_unlocked(data, sizeof(char), len, fp_) != len) {
            std::cerr << "fail to write file " << fileName_ << " error: " << strerror(errno) << std::endl;
        }
    }

    File(const File&) = delete;
    File& operator=(const File&) = delete;

private:
    std::string fileName_;
    FILE* fp_;
};

class LogFile {
public:
    LogFile(const std::string& fileName, size_t maxFileSize)
        : fileName_(fileName),
          maxFileSize_(maxFileSize) {
        rotate();
    }

    void write(const char* data, size_t len) {
        
        if (readSize_ + len > maxFileSize_) {
            rotate();
        }
        readSize_ += len;
        file_.write(data, len);
    }

    void setMaxFileSize(size_t millBytes) { maxFileSize_ = millBytes * kBytesPerMb; }
    // TODO: no thread safe. each thread can invoke it by LogAppender instance.
    void setFileName(const std::string& fileName) { 
        fileName_ = fileName; 
        rotate();
    }


private:
    std::string getFileName() {
        std::string name = fileName_;
        name += "-";
        name += detail::TimeStamp::Now().dateTime();
        std::replace(name.begin(), name.end(), ' ', '-');
        name += ".log";
        return name;
    }

    void rotate() {
        readSize_ = 0;
        file_.close();
        file_.open(getFileName());
    }

private:
    static const size_t kBytesPerMb = 1024 * 1024;
    File file_;
    std::string fileName_;
    size_t readSize_{0};
    size_t maxFileSize_;
};

} // qlog