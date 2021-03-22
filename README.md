
# 基于协程与事件驱动的网络库

用 c++11 编写，基于 linux 的 ucontext_t 封装了协程和调度器，并提供了简单的 tcp、http、rpc服务。

目前常见的网络库大多采用基于事件驱动和回调的机制，即将网络IO事件交给 select/poll/epoll 管理。（常用服务器模型见：https://zhuanlan.zhihu.com/p/93405607）
当网络事件到来时，执行对应的回调。尤其是事件驱动 + 线程池的设计，可以让并发量大大提高。但数据收发与逻辑的处理可能会被回调所分割，不利于业务逻辑的编写。
而目前许多语言和库都开始支持协程，通过协程可以向编写同步程序一样处理请求，使得代码更易读与维护。目前 go 语言从语言层面上支持了协程，通过 go func() 就可以将 func 中的一些可能阻塞的调用交由 go 的调度器处理。而一些出色的 C++ rpc 库也实现了协程的概念，如 brpc 和 phxrpc。此外，C++20 增加了无栈协程，后续在 C++23 也准备将协程引入到网络库中。因此计划通过实现一个基于协程的服务器来学习协程的原理与目前一些开源框架的设计思想。

## 编译

项目依赖于 protobuf 和 zlib，进入源目录下运行 auto_build.sh 即可将项目编译到 ./build/中

```bash
cd reyao
./auto_build.sh
```

## 日志库实现

在服务器编程中，应该尽量避免磁盘 IO 这种慢速的系统调用。而直接在工作线程写日志并写入磁盘，会频繁陷入系统调用，使得服务器的吞吐量收到很大影响。因此应该先把日志收集在缓冲区中，然后由日志线程定时的将缓冲区的数据写回磁盘。 

因此这是一个多生产者单消费者的问题，前端线程往日志线程的缓冲区写数据，后端线程定时读取缓冲区数据，所以需要线程同步机制，这里使用的是 mutex。

### 前端设计

采用 iostream 风格的流式输出，写日志时创建一个 LogWrapper 对象，返回一个流对象用于将数据写入 LogData 中保存。LogWrapper 析构时 LogData 传给 Logger 对象。Logger 对象对 LogData 进行格式化并发向日志目的地。

![image](https://user-images.githubusercontent.com/57951936/111903574-ec018600-8a7d-11eb-89f9-deee08354153.png)


### 后端设计

采用单独的日志线程，维护一块缓冲区定时将数据写回磁盘。需要关注的问题是，后端线程的缓冲区应该多大，如果多个前端线程短时间内写入大量数据怎么办？

- 为前端准备两块固定大小的缓冲区 A 和 B，如果 A 写完了，再交由后端统一处理，这样可以使得每次磁盘写的数据尽可能大。
- 如果短时间内 A 被写完，就换成另一块准备好的缓冲区 B 继续写，如果 B 也被写满，则重新分配内存。 
- 后端没数据时阻塞于条件变量，等待前端通知。同时每次处理完后复用刚写入磁盘的缓冲区，用于分配给前端写。

通过缓冲区的复用，可以避免由前端分配缓冲区，只需关注日志的写入，而把缓冲区的分配和释放交由给后端处理。

另一个问题是，如何保证日志线程的创建和摧毁的时机不会影响日志的正常写入？

- AsyncLog 对象负责后端线程的启动和日志的收集，所以在构造函数中启动线程时，应该同步等待线程启动，这个问题可以用线程同步解决。
- 最初版本的后台线程只是检测运行标志 running_，考虑到 AsyncLog 对象析构时不应该再有前端写入日志，这时如果后端正在数据落盘，然后检查标志退出，这时不再进入循环就会导致数据丢失，所以 AsyncLog 对象退出时应该再给后端一个机会写缓冲区数据：
  - 加入预退出标志，AsyncLog对象析构时先设置预退出标志 exit_ 然后通知后端，并等待后端线程的退出。后端线程下一次进入循环时发现 exit_ 标志，将 running_ 标志设置位 false，然后进行最后一次缓冲区的写入，进入下次循环时 running_ 为 false，退出。

### 吞吐量测试

测试机配置 CPU：锐龙4800U（8核16线程），内存：三星 8G * 2（3200Mhz），硬盘：WDC SN730
测试环境：WSL2
测试环境：WSL2

```
线程数----------每个线程消息数------消息大小(bytes)-----速度(mb/s)--------------------吞吐量（msg/sec)
10              1000000            33               29.6497			    944259
5               1000000            33               36.0358			    1.13347e+06
1               1000000            33               29.4888			    951253

10              1000000            73               42.6146			    612279
5               1000000            73               50.7368			    728977
1               1000000            73               54.5133			    790048

10              1000000            133              73.812			    582114
5               1000000            133              77.3407		 	    609943
1               1000000            133              87.2974			    692836
```

### 性能分析
可以看到日志库的吞吐量不是很大，分析主要有以下原因：

- 日志支持运行时改变 formatter 和 appender，导致每次写日志时获取这两个对象需要上锁，有性能损耗。
- 前端写线程会先 format 成字符串后往日志线程写，消耗前端线程的执行时间，考虑由日志线程统一作日志的格式化和持久化。
- 多线程往日志线程 append 数据会竞争锁，这是主要的性能瓶颈，且随着线程数的增加，每个线程的 CPU 利用率下降，说明线程有一部分时间在等待锁。能否改为更高效的前后端写入日志？

### 优化

- 无锁队列：前面实现的模型是多生产者单消费者，可以优化成 N 个单生产者单消费者队列。对于每一个前台线程，创建一个线程局部的环形缓冲区 RingBuffer。对于 RingBuffer，只需要维护队头和队尾指针，生产者移动队尾指针，消费者移动队头指针。这样前台线程写缓冲区和后台线程读缓冲区就不用上锁。对于多线程，后台线程只需遍历一遍所有线程的 RingBuffer 就能取到所有数据。
- RingBuffer 采用数组实现，每个线程只设置了 1M 大小的缓冲区，如果前台线程的消息大小很大，很快会填满缓冲区，这时直接让其轮询等待空闲空间，也有些日志库会丢弃最早的日志。
- 采用流式输出，直接输出到线程的缓冲区中，避免了消息格式化后再拷贝到缓冲区。
- 后端线程遍历所有线程缓冲区，取尽可能多的数据写回磁盘，减少陷入 write 的次数。
- 采用 C++ 11 的 atomic_thread_fench 实现内存屏蔽，避免由指令重排和缓存不一致导致缓冲区的错误读写。
- 利用线程同步与全局 Log 对象的析构确保程序结束时能够将所有缓冲区数据写入硬盘。

在相同环境下进行吞吐量测试：

```
线程数----------每个线程消息数------消息大小(bytes)-----速度(mb/s)------------吞吐量（msg/sec)
1               1000000            40               137.722		  3.6108e+06		  
5               1000000            40               153.994		  4.03778e+06
10              1000000            40               202.883		  5.31846e+06

1               1000000            80               207.548		  2.72037e+06
5               1000000            80               303.711		  3.98091e+06
10              1000000            80               403.372		  5.28708e+06

1               1000000            130              400.363		  3.22932e+06
5               1000000            130              480.431		  3.87515e+06
10              1000000            130              638.481		  5.15089e+06 

1               1000000            1030             1082.35		  1.10187e+06
5               1000000            1030             831.026		  846013
10              1000000            1030             372.636	          379356
```

对前端和后端重新设计后，可以看到随线程数增加，吞吐量明显提高。说明无锁的实现大大提高了日志的写入速度。但这样的设计瓶颈落在了缓冲区大小上，目前设置的是1 M。可以看到当消息大小为 1 KB 时，吞吐量下降很快。这是由于前端写入太快，导致 CPU 轮询的时间增加，且此时 CPU 负载为 100%。但对于小消息日志，在多线程下也有较好的性能表现。

## 协程与协程调度器实现

### 协程实现

协程是一种用户态线程。类似于线程，协程间可以共享地址空间、文件资源等等。但与线程不同，线程的调度由内核实现，需要陷入内核态进行切换。而协程是在用户态下进行切换，因此效率比线程切换要高。

目前 linux C++ 的协程库大多采用的是 stackfull 协程，即每一个协程实例有自己私有的堆栈。

linux 提供的系统 API 为 ucontext_t 族函数：

```c
#include <ucontext.h>

int getcontext(ucontext_t *ucp);
int setcontext(const ucontext_t *ucp);
void makecontext(ucontext_t *ucp, void (*func)(), int argc, ...);
int swapcontext(ucontext_t *oucp, const ucontext_t *ucp);

typedef struct ucontext_t {
  struct ucontext_t *uc_link;
  sigset_t          uc_sigmask;
  stack_t           uc_stack;
  mcontext_t        uc_mcontext;
               ...
} ucontext_t;
```

#### 上下文切换

参考：http://www.360doc.com/content/17/0928/09/19227797_690773203.shtml

getcontext 用于保存当前上下文信息。调用首先保存各个寄存器，然后保存 rip 即下一条要执行的指令地址，最后再保存当前的栈顶指针 rsp。当要恢复上下文时，恢复这些寄存器就能找到指令的位置和栈顶位置继续执行。**除了保存寄存器，还会设置浮点计数器和保存当前线程的信号屏蔽掩码，但这两个不是必要步骤，且 signal mask 会陷入内核，引起内核态和用户态的上下文切换，是 ucontext_t 族函数的性能瓶颈。**



![image](https://user-images.githubusercontent.com/57951936/111323249-a66f4280-86a4-11eb-9942-5f504a904941.png)

makecontext 用于修改当前的上下文信息，其接受函数指针和参数，然后把保存在上下文结构中的 rip 指针更换成函数指针的位置，这样在恢复这个上下文信息时就能跳转到指定的函数中执行。
![image](https://user-images.githubusercontent.com/57951936/111330831-4203b180-86ab-11eb-82d2-d0da3e436993.png)

ucontext_t 中的 uc_link 结构保存 ucontext_t 指针，在当前函数执行完后，会调用 setcontext 切换到 uc_link 指定的上下文继续执行。
![image](https://user-images.githubusercontent.com/57951936/111331360-ca825200-86ab-11eb-88b6-9165a1feec10.png)

swapcontext 接收两个 ucontext_t 指针，先用 getcontext 把当前的上下文信息保存到 oucp 中，然后用 setcontext 恢复 ucp 的上下文信息，流程与 getcontext 相反。
  
  
#### 协程栈

由上面的 API 可以看出，在设置 ucontext_t 时要指定协程栈，那么在协程对象很多时，怎么分配内存比较合理？

- 固定栈大小。如 linux 下给每个线程固定栈大小为 8 M。但如果协程使用的栈空间很大，会造成内存浪费，提高性能开销。而如果很小，则在协程内要避免分配大的内存空间，否则栈溢出时很难观察。（出现过，gdb 观察不到变量信息）。项目中的协程栈默认为128 k，在进行一般的 HTTP 和 RPC 请求时够用。但如果要在协程中引入第三方库如 mysql 等，还是有可能出现栈溢出。在分配内存时，可以采用 mmap 和 mprotect 对分配的内存进行保护，这也是 phxrpc 中的内存分配方法：
  -  mmap 分配 stack_size + 2 * page_size 大小，属性设置为可读写
  -  mprotect 将 分配的内存区域的首尾两个 page 设置 PROT_NONE 禁止访问，如果出现栈溢出则段错误。

##### phxrpc

phxrpc 的协程基本设计思路可以参考：https://cloud.tencent.com/developer/article/1398572。其协程基本对象为 uthread_context，提供 Make Resume Yield 接口进行上下文切换。协程对象统一由 uthread_runtime 管理，内部用 vector 管理所有协程对象：

```cpp
    struct ContextSlot {
        ContextSlot() {
            context = nullptr;
            next_done_item = -1;
        }
        UThreadContext * context;
        int next_done_item;
        int status;
    };
```

可以看到其 slot 节点除了有协程对象，还有下一个 next_done_item。此外 runtime 对象还记录了 first_done_item 的指针。这两个指针巧妙地实现了协程对象的复用：

- 调用 Create 接口创建一个协程对象时，会找到 first_done_item，然后将这个 slot 的协程对象用于分配给新协程，避免内存分配。随后更新 first_done_item。
- 如何更新？first_done_item 的 slot 节点的 next_done_item 指向下一个执行完的协程对象，等于是一个完成对象的链表。每当协程对象执行完时将该对象插入链表头，每次创建对象时从链表头取 slot 对象。

phxrpc 通过 vector + list 的组合实现了协程对象复用，但依旧没有解决固定栈带来的隐患，用户代码在栈空间分配时需要谨慎。

其余方案参考：https://www.jianshu.com/p/837bb161793a

- 分段栈方案

glibc 提供一种编译参数，用于在函数调用时插入一段检测代码，如果栈空间不够就分配一块新的内存，但如果引入的第三方库没有这样编译，则无法使用。

- 拷贝栈

内存不够分配一块新内存拷贝过去，但 C++ 语言层面难以实现，因为内存重分配会导致原先栈空间内存失效。

- 共享栈

libco 的协程栈实现采用共享栈，具体实现参考：https://www.zhihu.com/question/52193579的钟宇腾回答。

即协程有一个私有栈空间，可以动态变化：

- 初始化协程时，调用 co_alloc_sharestack 从共享栈中取一块内存作为栈的私有栈空间。

- yield 时，调用 save_stack_buffer,其计算当前所用共享栈 ebp 到 esp 的距离，然后释放原有协程栈，调用 malloc 和 memcpy 将共享栈的内容拷贝到私有栈，这个备份放在协程结构 stCoroutine_t 中。
- 协程要 resume 时，从共享栈再取一块内存，调用 memcpy 将协程结构的私有栈拷贝过去。

具体实现在 co_routine.cpp 的 sav_stack_buffer() 和 co_swap() 中

只要共享栈分配的内存空间足够大，就不会栈溢出，然后通过切换时的 malloc/free 和 memcpy 更新协程的私有栈。

当然动态分配也要付出一些代价：

- 协程栈大了之后，memcpy 的开销变大，但大多数协程用的栈空间较小，所以拷贝的代价不是很大。
- C++ 有引用和指针，当协程栈通过 malloc & memcpy 重分配空间时，指向原栈空间的指针和引用都会失效，引起未定义行为。

#### 项目中的实现

目前采用固定栈的方式，默认栈大小为 128 K，设置内存保护，正常使用中很少会溢出。

### 调度器实现

Coroutine 对象只是封装了系统 API，但在程序中直接使用来切换上下文会程序逻辑变得复杂，在此基础上实现了 Worker 类用于协程的调度。

Worker 类在线程内调度的逻辑：

- 线程从 Worker::run 开始，先初始化主协程（getcontext 获得当前上下文）t_main_coroutine，主协程是线程局部变量，用于线程内的协程切换工作，它从任务队列中取出任务执行然后 resume 进去执行，协程退出或挂起都采用 yield 切换回主协程，然后再取任务，协程调度流程为:

![image](https://user-images.githubusercontent.com/57951936/111908943-2119d280-8a96-11eb-8e24-2cf4638ea976.png)


- 除了 t_main_coroutine，还有线程局部对象 t_cur_coroutine，表示当前正在运行的协程。协程切换时，实际是用 t_cur_coroutine 和 t_main_coroutine 中来回切换

- 主协程在取任务，任务可以是 function 或协程对象：

  ```cpp
  struct Task {
          Func func = nullptr;
          Coroutine::SPtr co = nullptr;
  };
  ```

  - 如果是协程对象，则直接 resume 执行，当切回主协程时，这个协程对象可能有两种状态：1. EXIT：可以退出。2.SUSPEND：协程等待在某些队列中，然后 yield。不管是哪种情况，主协程都不关心这个协程的后续状态，调用 task.reset() 释放其任务。
  - 如果 Task 是 function，主协程创建新的协程对象 co_task，resume 进去执行，同理 yield 回来的时候需要释放 task 中的任务。如果 co_func 为 EXIT，表示协程对象的 Func 执行完了，不立刻释放 co_func，而是在下次主协程取到 Func 任务时直接复用 co_func。
  - 如果无 Task，则切换到 idle 协程。idle 协程内部是死循环，内部执行 epoll_wait。首先从定时器从选择最近超时时间作为 epoll_wait 的等待时间，然后 epoll_wait 等待注册事件，返回后处理超时事件和读写事件，最后 yield 会主协程。

## hook 配合调度器封装 socket 函数

socket 函数默认是阻塞的，如果放在协程里跑，直接阻塞整个线程。所以为了将网络 IO 与协程库结合使用，需要 hook socket 函数。

- sockfd 读写

当在一个协程中读写 sockfd，如果不能立刻完成，则将 sockfd 和当前协程封装到 IOEvent 中发送给 Epoller 注册相应的 IO 事件。然后协程 yield 出去。当 IO 事件到来时， Epoller 会从对应的 IOEvent 结构中取到挂起的协程重新添加到调度器中，调度器继续执行挂起的协程。

为了完成这样的操作，sockfd 必须是非阻塞的，同时对于一个 sockfd 的读写，不能一直等待直到 tcp_keepalive 机制将其 kill 掉，这样如果客户端掉了，需要等待两个小时。

- hook

1. 可以用 dlsym(3) 来获取想要hook的函数的函数指针，先保存起来，如果想要用到原函数，可以通过保存的函数指针进行调用。
2. 定义自己的同名函数，覆盖想要 hook 的函数。以 sleep(3) 为例。

- sockfd 超时管理

用 FdManager 管理所有的 sockfd，其中有 sockfd 的读写超时时间。在调用 socket 分配 fd 时，在 fd 管理器中创建一个对应结构，里面记录 fd 的读写超时时间。当拿到一个 sockfd 时可以调用 hook 过的 setsockopt 设置 SO_RCVTIMEO 和 SO_SNDTIMEO 标志，在函数内会将设置的超时时间记录在 FdManager 的 FdContext 中。

在对 sockfd 读写如果不能立刻完成，在将 sockfd 注册到 Epoller 之前，先添加一个定时事件，当超时触发，会设置一个取消事件的标志，然后通知 Epoller 取消相应事件的监听，并直接将 IOEvent 中的协程返回调度器。当协程继续执行时，会检测到取消的标志，然后将 errno 设置为 ETIMEOUT 直接返回 -1。

这个取消标志为

```cpp
struct timer_info {
    int cancelled = 0;
};
```
首先创建一个timer_info 的智能指针，然后将其 weak_ptr 传给定时器。如果在超时前可以读写，那么 epoll 会先将协程放入调度器中先调度，协程切换回来后发现会先取消定时事件，然后检查 weak_ptr 中的 cancelled 标志。之所以设置成 weak_ptr ，是因为定时器统一由 Scheduler 管理，假设一个 fd 在线程 A 的 epoll 中等待，在超时时间时读写事件来，此时线程 B 刚好也从 epoll 唤醒，线程 B 抢先拿到了锁，从 Scheduler 中取出所有超时事件。随后线程 A 将所有触发的协程加入任务队列，此时协程会取消 timer 失败，然后退出。随后超时事件被调度，它要检查一个已退出协程的局部变量，此时是危险行为。所以为了避免这种小概率事件，用 weak_ptr 来避免两个线程分别处理读写协程和超时事件。

有了定时事件的管理，还可以 hook connect，设置超时时间。

- 用户设置非阻塞

正常的非阻塞行为应该是立刻返回，在 FdContext 中添加了 user_non_block 标志，如果用户主动调用 fnctl 或 ioctl 设置非阻塞，就修改 user_non_block 标志。这样在读写失败时不会交给 epoll 等待，而是直接返回，以模拟其原来的行为。


## Http 解析

HTTP请求格式：
```

method[GET] scheme://host:port/path?query#fragment version'\0''\0'\r\n

HEADER_NAME: content\r\n

...

\r\n

BODY
```


HTTP响应格式：
```

200 RSP_STR\r\n

HEADER_NAME: content\r\n

...

\r\n

BODY
```


由于请求和响应都分为三部分，所以采用状态机的方法解析请求和响应：

分为三种状态

```cpp
    enum ParseState {
        PARSE_FIRST_LINE,
        PARSE_HEADER,
        PARSE_BODY
    };
```

当有数据可读，检查当前处于哪种状态，然后进入相应处理逻辑。如果在处理时发现读取不完整，退出函数继续读 sockfd，直到有新的数据再解析。


HTTP/1.1用 content_length 规定 body 的长度，所以在 PARSE_BODY 时会一直读直到满足 content_length 为止。

但如果设置了 transfer-encoding: CHUNKED，则表示分段发送 body。此时先解析长度（16进制），再读取内容，直到读取到长度为0表示body发送完毕。为了分段解析，也设置不同状态，根据状态处理不同逻辑

```cpp
enum ParseChunkState {
        PARSE_CHUNK_SIZE,
        PARSE_CHUNK_CONTENT
    };
```

### GET请求

get 请求可以请求服务器上的资源，项目采用 ServletDispatch 对请求资源进行单独处理。

ServletDispatch 根据请求的 path 查找相应的 Servlet 进行，Servlet 封装了 path 和对应的 handle 处理函数，handle 函数进行请求接收和发送响应
ServletDispatch 先尝试精确匹配，如果找不到再进行模糊匹配

模糊匹配用的是 fnmatch：int fnmatch(const char *pattern, const char *string, int flags);

如 pattern 是 "/file1"，而 string 是 "file1/file2"，这时找不到 string ，就会匹配到 pattern 上。


### 压测
与 muduo 进行了 echo hello 的吞吐量测试，muduo 与其他网络库的性能做过对比：https://blog.csdn.net/qq_41453285/article/details/107018098

muduo 长连接

| 线程数 |   10   |  100   |    500    |   1000    |
| :----: | :----: | :----: | :-------: | :-------: |
|   1    | 39581  | 38470  |   40194   |   43094   |
|   4    | 120142 | 117830 |  122869   |  139006   |
|   8    | 107092 | 248835 | 部分 fail | 部分 fail |

muduo 长连接下当线程数等于1或4时，工作线程的负载为99%，连接线程负载为0%。8个线程时工作线程负载在70%到95%之间。且并发数大于500时出现异常连接，看日志发现 accept ：too many open file 和 TcpConnection error：broken pipe or RST。

reyao 长连接

| 线程数 |  10   |  100   |  500   |  1000  |
| :----: | :---: | :----: | :----: | :----: |
|   1    | 30000 | 28881  | 29610  | 31418  |
|   4    | 92747 | 91210  | 91509  | 99340  |
|   8    | 66523 | 142304 | 141328 | 157669 |

reyao 长连接下当线程数等于1或4时，工作线程的负载为99%，连接线程负载为0%，这一点与 muduo 一致。8个线程时工作线程负载平均值在90左右之间。4 个工作线程时，muduo 长连接的 QPS 高了~35%，八个线程时高了~70%。

分析原因如下：

长连接时是一次性建立所有连接，然后不断往连接发请求。这时 sockfd 收到的数据是很多个请求加在一起， reyao 目前在处理 HTTP 请求时最多一次读 4 k 数据到缓冲区，而 muduo 在读 sockfd 会一次读 16 k 数据，读出来的数据先放到 TcpConnection 的 Buffer 中（1 k），多余的直接放在栈中的 extrabuf，然后在对 Buffer 扩容。因此 muduo 每一读了 4 倍的数据量，系统调用的次数远少于 reyao，因此在长连接时性能会高很多。这一点 muduo 在与 libevent 对比时也指出过。

muduo 短连接

| 线程数 |  10   |  100  |  500  | 1000  |
| :----: | :---: | :---: | :---: | :---: |
|   1    | 22647 | 22880 | 24413 | 26413 |
|   4    | 39779 | 56943 | 58788 | 65298 |
|   8    | 21882 | 22895 | 24494 | 27114 |

reyao 短连接

| 线程数 |  10   |  100  |  500  | 1000  |
| :----: | :---: | :---: | :---: | :---: |
|   1    | 20104 | 23950 | 25472 | 27315 |
|   4    | 39720 | 47161 | 52085 | 58172 |
|   8    | 32415 | 32678 | 36553 | 40705 |

短连接测试时，在线程数为1时，QPS差距不大。线程数为4时，muduo 的 QPS 高了 12%。线程数为8时，reyao 的 QPS 略高一筹。观察 CPU 负载时发现，muduo 4个线程时 CPU 负载为 92%，8个线程时负载为42%。reyao 4个线程时负载为87%，8个线程时负载为43~45%。

性能分析：

muduo 采用 LT 模式，且 listenfd 每次事件到来时只 accept 一次就返回，当并发请求较多时，会频繁的 epoll_wait 而消耗大量时间，并发量1000时观察 CPU 负载时发现，muduo 的连接线程的负载一直为 95% 以上。而 reyao 采用 ET 模式，每次有新的请求到来，都会一直读直到 EAGAIN，所以每次 accept 的请求比较多，reyao 的连接线程负载为 75~80%。

muduo 采用回调的方式处理读写事件，效率较高；而 reyao 在处理不同 sockfd 时需要协程切换，要从 Co1 -> mainCo -> Co2 两次切换，虽然没有陷入内核，但两次保存寄存器和两次恢复寄存器的操作还是拖累了表现。且 linux 的 getcontext 在保存寄存器时还做了额外操作（信号屏蔽处理），性能并不是很好。phxrpc 对比过基于 ucontext 与基于 boost 的网络请求性能测试（https://github.com/Tencent/phxrpc）。


## RPC 
rpc 调用就是客户端将方法名 参数名 参数值等内容发送给服务端，由服务端进行方法调用。

由于传输层传输的二进制串，所以需要将客户端中的对象转化为二进制格式传给服务端，而服务端要将二进制格式转化为对象，这就是序列化和反序列化的过程。

但是 C++ 并没有反射的机制，不能在运行时通过类型名字节生成类的对象并调用类的方法。

当然有一些简单的方法，如建立一个 map，key 为 类型名，value 是一个回调，分配一个对象。然后用宏包装一下，就可以只传递类型名加入将回调加到 map 中。

而更通用的做法是使用 IDL（interface description language），可以让通信双方对通信内容进行约定。IDL 最大的好处是平台无关，它依赖于 IDL 编译器，将 IDL 文件生成不同语言对于的库。

protobuf 使用：

假如在 proto 文件中定义了一个 message 类型，使用 protoc 工具生成对应的头文件和源文件，里面会生成对应的 类，提供类成员的 get set 方法。

```bash
protoc -I=. --cpp_out=. my.proto
```

protobuf通过Descriptor获取任意message或service的属性和方法，Descriptor主要包括了一下几种类型：

| descroptor        | Usage                                                        |
| ----------------- | :----------------------------------------------------------- |
| FileDescriptor    | 获取Proto文件中的Descriptor和ServiceDescriptor               |
| Descriptor        | 获取类message属性和方法，包括FieldDescriptor和EnumDescriptor |
| FieldDescriptor   | 获取message中各个字段的类型、标签、名称等                    |
| EnumDescriptor    | 获取Enum中的各个字段名称、值等                               |
| ServiceDescriptor | 获取service中的MethodDescriptor                              |
| MethodDescriptor  | 获取各个RPC中的request、response、名称等                     |

也就是说，如果能够获取到proto文件的 FileDescriptor ，就能获取 proto 文件中的所有的内容，或者得到 descriptor 获得 message 信息。

- 获取 FileDescriptor：

直接用 proto 文件生成头文件和源文件，并获取 FileDescriptor

```cpp
const FileDescriptor* fileDescriptor = DescriptorPool::generated_pool()->FindFileByName(file);
```

- 获取 Descriptor

通过 Message 名获得 Descriptor

```cpp
const Descriptor* descriptor = DescriptorPool::generated_pool()->FindMessageTypeByName(typeName);
```

通过 FileDescriptor 获得Descriptor

```cpp
const Descrpitor descriptor = fileDescriptor->FindMessageTypeByName(typeName);
```



获取 Message 的 Desrciptor 后，可以获取其默认实例，这时每个 Message Type 生成头文件和源代码之后都会有的。

```cpp
const Message* prototype = MessageFactory::generated_factory()->GetPrototype(descriptor);
```

获取 Message 的默认实例后，通过 New 方法创建一个相同类型的对象实例

```cpp
Message* msg = prototype->New();
```

获得实例后，便可以通过 set get 方法读写该对象。同时这个对象是动态分配的，最好用 shared_ptr 管理。

Rpc 注册：

RpcServer 提供 registerRpcHandler 注册 Message 对象的调用方法，用模板函数实现，模板为一个 Message Type，接收一个对应类型的回调，生成一个对应的RpcCallBack<MessageType> 实例，其中保存了回调函数。当 server 收到 Message 对象时，通过查找类型的 Descriptor 找到对应的 RpcCallBack 实例，调用其回调得到一个 Message 对象发送给调用方。

```cpp
template <class T>
void registerRpcHandler(const typename RpcCallBackType<T>::MessageCallBack& handler) {
    std::shared_ptr<RpcCallBackType<T>> cb(new RpcCallBackType<T>(handler));
    MutexGuard lock(mutex_);
    handlers_[T::descriptor()] = cb;
}

class RpcCallBack {
public:
    virtual ~RpcCallBack() = default;

    virtual MessageSPtr onMessage(const MessageSPtr& msg) = 0;
};

template <class T>
class RpcCallBackType : public RpcCallBack {
    static_assert(std::is_base_of<google::protobuf::Message, T>::value,
                  "T must be sub-class of google::protobuf::Message");

public:
    typedef std::function<MessageSPtr(const std::shared_ptr<T> &)> MessageCallBack;
    RpcCallBackType(const MessageCallBack &cb) : cb_(cb) {}
    MessageSPtr onMessage(const MessageSPtr &msg) {
        std::shared_ptr<T> m = std::static_pointer_cast<T>(msg); // base到derive的转换
        return cb_(m);
    }

private:
    MessageCallBack cb_;
};
```

Rpc 请求：

RpcClient 可以通过 Call<T> 发起 rpc 请求，其形参接收一个 req Message，同时还要接收一个 rsp Message 的回调，用于接收到响应后执行响应的处理。

```cpp
template<typename RspMessage>
class TypeTraits {
    static_assert(std::is_base_of<::google::protobuf::Message, RspMessage>::value, 
                  "T must be subclass of google::protobuf::Message");
public:
    typedef std::function<void(std::shared_ptr<RspMessage>)> ResponseHandler;
};


template<typename RspMessage>
bool Call(MessageSPtr req, typename TypeTraits<RspMessage>::ResponseHandler handler);
```

Reference：
https://github.com/sylar-yin/sylar

https://github.com/chenshuo/muduo

https://github.com/gatsbyd/melon

