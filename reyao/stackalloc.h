#pragma once

#include <sys/types.h>

namespace reyao {

class StackAlloc {
public:
    StackAlloc(size_t stack_size, bool protect = true);
    ~StackAlloc();

    void* top();
    size_t size();
private:
    void* raw_stack_ = nullptr;
    void* stack_ = nullptr;
    size_t stack_size_;
    bool protect_;
};

} //namespace reyao