#pragma once

#include "reyao/mutex.h"
#include "reyao/nocopyable.h"

#include <pthread.h>
#include <time.h>

#include <functional>
#include <memory>

namespace reyao {

class Thread : public NoCopyable {
public:
	typedef std::shared_ptr<Thread> SPtr;
	typedef std::function<void ()> ThreadFunc;

	explicit Thread(ThreadFunc cb, const std::string& name);
	~Thread();

	static void SetThreadName(const std::string& name);
    static const std::string& GetThreadName();
    static pid_t GetThreadId();
	static void* RunThread(void* arg);

	void start();
	void join();
	pid_t getId() const { return id_;}

private:
	pthread_t tid_;          	//线程结构体
	pid_t  id_;              	//线程id
	ThreadFunc cb_;             //回调函数
    std::string name_;   		//线程名称
    CountDownLatch latch_;      //确保线程启动
	bool started_;       		//线程开始标志
	bool joined_;        		//线程回收标志
}; 

} //namespace reyao