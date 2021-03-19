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
	bool isStart() const { return running_; }

private:
	pthread_t tid_; 
	pid_t  id_; 
	ThreadFunc cb_; 
    std::string name_;  
    CountDownLatch latch_;
	bool running_;       
	bool joined_;   
}; 

} // namespace reyao