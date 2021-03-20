#pragma once

#include <sys/types.h>

namespace reyao {

class StackAlloc {
public:
    StackAlloc(size_t stackSize, bool protect = true);
    ~StackAlloc();

    void* top() {
        return stack_;
    }
    size_t size() {
        return stackSize_;
    }
private:
    void* rawStack_ = nullptr;
    void* stack_ = nullptr;
    size_t stackSize_;
    bool protect_;
};

} //namespace reyao