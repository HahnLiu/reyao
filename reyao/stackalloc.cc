#include "reyao/stackalloc.h"

#include <assert.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>

namespace reyao {

StackAlloc::StackAlloc(size_t stack_size, bool protect):
    stack_size_(stack_size),
    protect_(protect) {
    if (stack_size != 0) {
        if (protect_) {
            int page_size = getpagesize();
            if (stack_size % page_size != 0) {
                stack_size_ = (stack_size / page_size + 1) * page_size;
            } else {
                stack_size_ = stack_size;
            }

            raw_stack_ = mmap(NULL, stack_size_ + page_size * 2,
                              PROT_READ | PROT_WRITE,
                             MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
            assert(raw_stack_ != nullptr);
            assert(mprotect(raw_stack_, page_size, PROT_NONE) == 0);
            assert(mprotect((void*)((char*)raw_stack_ + stack_size_ + page_size), page_size, PROT_NONE) == 0);
            stack_ = (void*)((char*)raw_stack_ + page_size);
        } else {
            stack_ = malloc(stack_size_);
        }
    } else {
        stack_size_ = 0;
    }
}

StackAlloc::~StackAlloc() {
    if (stack_size_ != 0) {
        if (protect_) {
            int page_size = getpagesize();
            assert(mprotect(raw_stack_, page_size, PROT_READ | PROT_WRITE) == 0);
            assert(mprotect((void *)((char *)raw_stack_ + stack_size_ + page_size), page_size, PROT_READ | PROT_WRITE) == 0);
            assert(munmap(raw_stack_, stack_size_ + page_size * 2) == 0);
        } else {
            free(stack_);
        }

    }
}

void* StackAlloc::top() {
    return stack_;
}

size_t StackAlloc::size() {
    return stack_size_;
}

} //namespace reyao