#pragma once

#include "reyao/nocopyable.h"

#include <string.h>

#include <string> 
#include <memory>

namespace reyao {
    
template <size_t SIZE>
class FixedBuffer : public NoCopyable {
public:

    void append(const char* data, size_t len) {   
        if (SIZE - pos_ >= len) {
            memcpy(data_ + pos_, data, len);
            pos_ += len;
        }
    }

    void append(const std::string& data) {
        size_t len = data.size();
        append(data.c_str(), len);
    }

    const char* data() {
        return data_;
    }

    size_t avail() { 
        return SIZE - pos_ ; 
    }

    size_t size() { 
        return pos_; 
    }

    void reset() { 
        pos_ = 0; 
    }
private:
    char data_[SIZE];
    size_t pos_ = 0;
};

} //namespace reyao