#include "reyao/stackalloc.h"

#include <assert.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>

namespace reyao {

StackAlloc::StackAlloc(size_t stackSize, bool protect):
    stackSize_(stackSize),
    protect_(protect) {
    if (stackSize != 0) {
        if (protect_) {
            int pageSize = getpagesize();
            if (stackSize % pageSize != 0) {
                stackSize_ = (stackSize / pageSize + 1) * pageSize;
            } else {
                stackSize_ = stackSize;
            }

            rawStack_ = mmap(NULL, stackSize_ + pageSize * 2,
                             PROT_READ | PROT_WRITE,
                             MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
            assert(rawStack_ != nullptr);
            assert(mprotect(rawStack_, pageSize, PROT_NONE) == 0);
            assert(mprotect((void*)((char*)rawStack_ + stackSize_ + pageSize), 
                            pageSize, PROT_NONE) == 0);
            stack_ = (void*)((char*)rawStack_ + pageSize);
        } else {
            stack_ = malloc(stackSize_);
        }
    } else {
        stackSize_ = 0;
    }
}

StackAlloc::~StackAlloc() {
    if (stackSize_ != 0) {
        if (protect_) {
            int pageSize = getpagesize();
            assert(mprotect(rawStack_, pageSize, PROT_READ | PROT_WRITE) == 0);
            assert(mprotect((void *)((char *)rawStack_ + stackSize_ + pageSize), 
                            pageSize, PROT_READ | PROT_WRITE) == 0);
            assert(munmap(rawStack_, stackSize_ + pageSize * 2) == 0);
        } else {
            free(stack_);
        }

    }
}


} //namespace reyao