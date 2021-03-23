
# 基于协程与事件驱动的网络库

用 c++11 编写，基于 linux 的 ucontext_t 封装了协程和调度器，并提供了简单的 tcp、http、rpc服务。

## 编译

项目依赖于 protobuf 和 zlib，进入源目录下运行 auto_build.sh 即可将项目编译到 ./build/中

```bash
cd reyao
./auto_build.sh
```

## 项目介绍
1、采用无锁环形队列实现异步日志库，支持自定义日志格式与日志目的地，易于拓展；

2、基于ucontext实现了有栈协程与非抢占式的调度器，支持单线程下协程间同步机制；

3、采用epoll边缘触发的 IO复用技术实现了N-M的协程调度器；

4、Hook socket API和sleep让同步的网络IO展现出异步的性能，支持设置连接超时；

5、实现了简单的HTTP/1.1协议，支持GET请求与长连接，支持uri的精确匹配和模糊匹配；使用Protobuf作为IDL实现RPC调用。


## 日志库吞吐量测试

测试机配置 CPU：锐龙4800U（8核16线程），内存：三星 8G * 2（3200Mhz），硬盘：WDC SN730
测试环境：WSL2
测试环境：WSL2

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
10              1000000            1030             372.636	    379356
```

可以看到随线程数增加，吞吐量明显提高。说明无锁的实现大大提高了日志的写入速度。但这样的设计瓶颈落在了缓冲区大小上，目前设置的是 1M。可以看到当消息大小为 1KB 时，吞吐量下降很快。这是由于前端写入太快，导致 CPU 轮询的时间增加，且此时 CPU 负载为100%。但对于小消息日志，在多线程下也有较好的性能表现。

## 协程与协程调度器实现

### 协程实现

协程是一种用户态线程。类似于线程，协程间可以共享地址空间、文件资源等等。但与线程不同，线程的调度由内核实现，需要陷入内核态进行切换。而协程是在用户态下进行切换，因此效率比线程切换要高。目前 linux C++ 的协程库大多采用的是 stackfull 协程，即每一个协程实例有自己私有的堆栈。linux 提供的系统 API 为 ucontext_t 族函数：
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
  
#### 协程栈
- 固定栈大小。如 linux 下给每个线程固定栈大小为 8 M。但如果协程使用的栈空间很大，会造成内存浪费，提高性能开销。而如果很小，则在协程内要避免分配大的内存空间，否则栈溢出时很难观察。（出现过，gdb 观察不到变量信息）。项目中的协程栈默认为 128 k，在进行一般的 HTTP 和 RPC 请求时够用。但如果要在协程中引入第三方库时，还是有可能出现栈溢出。在分配内存时，可以采用 mmap 和 mprotect 对分配的内存进行保护，这也是 phxrpc 中的内存分配方法：
  -  mmap 分配 stack_size + 2 * page_size 大小，属性设置为可读写
  -  mprotect 将 分配的内存区域的首尾两个 page 设置 PROT_NONE 禁止访问，如果出现栈溢出则段错误。

其余方案参考：https://www.jianshu.com/p/837bb161793a
- 分段栈方案

glibc 提供一种编译参数，用于在函数调用时插入一段检测代码，如果栈空间不够就分配一块新的内存，但如果引入的第三方库没有这样编译，则无法使用。

- 拷贝栈

内存不够分配一块新内存拷贝过去，但 C++ 语言层面难以实现，因为内存重分配会导致原先栈空间内存失效。

- 共享栈

libco 的协程栈实现采用共享栈，具体实现参考：https://www.zhihu.com/question/52193579

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

## hook 配合调度器封装 socket 函数

socket 函数默认是阻塞的，如果放在协程里跑，直接阻塞整个线程。所以为了将网络 IO 与协程库结合使用，需要 hook socket 函数。

- sockfd 读写

当在一个协程中读写 sockfd，如果不能立刻完成，则将 sockfd 和当前协程封装到 IOEvent 中发送给 Epoller 注册相应的 IO 事件。然后协程 yield 出去。当 IO 事件到来时， Epoller 会从对应的 IOEvent 结构中取到挂起的协程重新添加到调度器中，调度器继续执行挂起的协程。为了完成这样的操作，sockfd 必须是非阻塞的，同时对于一个 sockfd 的读写，不能一直等待直到 tcp_keepalive 机制将其 kill 掉，这样如果客户端掉了，需要等待两个小时，因此需要设置连接超时时间。

- hook

1. 可以用 dlsym(3) 来获取想要hook的函数的函数指针，先保存起来，如果想要用到原函数，可以通过保存的函数指针进行调用。
2. 定义自己的同名函数，覆盖想要 hook 的函数。以 sleep(3) 为例。

- sockfd 超时管理

用 FdManager 管理所有的 sockfd，其中有 sockfd 的读写超时时间。在调用 socket 分配 fd 时，在 fd 管理器中创建一个对应结构，里面记录 fd 的读写超时时间。当拿到一个 sockfd 时可以调用 hook 过的 setsockopt 设置 SO_RCVTIMEO 和 SO_SNDTIMEO 标志，在函数内会将设置的超时时间记录在 FdManager 的 FdContext 中。

在对 sockfd 读写如果不能立刻完成，在将 sockfd 注册到 Epoller 之前，先添加一个定时事件，当超时触发，会设置一个取消事件的标志，然后通知 Epoller 取消相应事件的监听，并直接将 IOEvent 中的协程返回调度器。当协程继续执行时，会检测到取消的标志，然后将 errno 设置为 ETIMEOUT 直接返回 -1。有了定时事件的管理，就可以在处理网络连接与 IO 时超时时间。

- 用户设置非阻塞

正常的非阻塞行为应该是立刻返回，在 FdContext 中添加了 userNonBlock 标志，如果用户主动调用 fnctl 或 ioctl 设置非阻塞，就修改 userNonBlock 标志。这样在读写失败时不会交给 epoll 等待，而是直接返回，以模拟其原来的行为。

## Http 解析
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
get 请求可以请求服务器上的资源，项目采用 ServletDispatch 对请求资源进行单独处理。ServletDispatch 根据请求的 path 查找相应的 Servlet 进行，Servlet 封装了 path 和对应的 handle 处理函数，handle 函数进行请求接收和发送响应。ServletDispatch 先尝试精确匹配，如果找不到再进行模糊匹配。

### 压测
与 muduo 进行了 echo hello 的吞吐量测试，muduo 与其他网络库的性能做过对比：https://blog.csdn.net/qq_41453285/article/details/107018098

muduo 长连接

| 线程数 |   10   |  100   |    500    |   1000    |
| :----: | :----: | :----: | :-------: | :-------: |
|   1    | 39581  | 38470  |   40194   |   43094   |
|   4    | 120142 | 117830 |  122869   |  139006   |
|   8    | 107092 | 248835 | 部分 fail | 部分 fail |

muduo 长连接下当线程数等于1或4时，工作线程的负载为99%，连接线程负载为0%。8个线程时工作线程负载在70%到95%之间。且并发数大于500时出现异常连接，看日志发现 accept ：too many open file 和 TcpConnection error：broken pipe or RST。

本项目长连接

| 线程数 |  10   |  100   |  500   |  1000  |
| :----: | :---: | :----: | :----: | :----: |
|   1    | 30000 | 28881  | 29610  | 31418  |
|   4    | 92747 | 91210  | 91509  | 99340  |
|   8    | 66523 | 142304 | 141328 | 157669 |

本项目长连接下当线程数等于1或4时，工作线程的负载为99%，连接线程负载为0%，这一点与 muduo 一致。8个线程时工作线程负载平均值在90左右之间。4 个工作线程时，muduo 长连接的 QPS 高了~35%，八个线程时高了~70%。

分析原因如下：

长连接时是一次性建立所有连接，然后不断往连接发请求。这时 sockfd 收到的数据是很多个请求加在一起， 本项目目前在处理 HTTP 请求时最多一次读 4k 数据到缓冲区，而 muduo 在读 sockfd 会一次读 16 k 数据，读出来的数据先放到 TcpConnection 的 Buffer 中（1 k），多余的直接放在栈中的 extrabuf，然后在对 Buffer 扩容。因此 muduo 每一读了 4 倍的数据量，系统调用的次数远少于本项目，因此在长连接时性能会高很多。这一点 muduo 在与 libevent 对比时也指出过。

muduo 短连接

| 线程数 |  10   |  100  |  500  | 1000  |
| :----: | :---: | :---: | :---: | :---: |
|   1    | 22647 | 22880 | 24413 | 26413 |
|   4    | 39779 | 56943 | 58788 | 65298 |
|   8    | 21882 | 22895 | 24494 | 27114 |

本项目短连接

| 线程数 |  10   |  100  |  500  | 1000  |
| :----: | :---: | :---: | :---: | :---: |
|   1    | 20104 | 23950 | 25472 | 27315 |
|   4    | 39720 | 47161 | 52085 | 58172 |
|   8    | 32415 | 32678 | 36553 | 40705 |

短连接测试时，在线程数为1时，QPS差距不大。线程数为4时，muduo 的 QPS 高了 12%。线程数为8时，本项目的 QPS 略高一筹。观察 CPU 负载时发现，muduo 4个线程时 CPU 负载为 92%，8个线程时负载为42%。reyao 4个线程时负载为87%，8个线程时负载为43~45%。

性能分析：

muduo 采用 LT 模式，且 listenfd 每次事件到来时只 accept 一次就返回，当并发请求较多时，会频繁的 epoll_wait 而消耗大量时间，并发量1000时观察 CPU 负载时发现，muduo 的连接线程的负载一直为 95% 以上。而本项目采用 ET 模式，每次有新的请求到来，都会一直读直到 EAGAIN，所以每次 accept 的请求比较多，reyao 的连接线程负载为 75~80%。

muduo 采用回调的方式处理读写事件，效率较高；而本项目在处理不同 sockfd 时需要协程切换，要从 Co1 -> mainCo -> Co2 两次切换，虽然没有陷入内核，但两次保存寄存器和两次恢复寄存器的操作还是拖累了表现。且 linux 的 getcontext 在保存寄存器时还做了额外操作（信号屏蔽处理），性能并不是很好。phxrpc 对比过基于 ucontext 与基于 boost 的网络请求性能测试（https://github.com/Tencent/phxrpc）。


## RPC 
rpc 调用就是客户端将方法名 参数名 参数值等内容发送给服务端，由服务端进行方法调用。由于传输层传输的二进制串，所以需要将客户端中的对象转化为二进制格式传给服务端，而服务端要将二进制格式转化为对象，这就是序列化和反序列化的过程。通用的做法是使用 IDL（interface description language），可以让通信双方对通信内容进行约定。IDL 最大的好处是平台无关，它依赖于 IDL 编译器，将 IDL 文件生成不同语言对于的库。

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

https://github.com/Tencent/phxrpc

https://github.com/Tencent/libco

