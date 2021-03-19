#include "reyao/thread.h"
#include "reyao/log.h"

#include <unistd.h>
#include <sys/syscall.h>

namespace reyao {

thread_local std::string t_threadName = "MainThread";
thread_local pid_t t_threadId = 0;


Thread::Thread(ThreadFunc cb, const std::string& name)
	: tid_(0),
	  id_(0),
	  cb_(cb),
	  name_(name),
	  latch_(1),
	  running_(false),
	  joined_(false) {

}

Thread::~Thread() {
	if (running_ && !joined_) {
		pthread_detach(tid_);
	}
}

void Thread::SetThreadName(const std::string& name) {
	t_threadName = name;
}

const std::string& Thread::GetThreadName() {
	return t_threadName;
}

pid_t Thread::GetThreadId() {
	if (t_threadId == 0) {
		t_threadId = syscall(SYS_gettid);
	}
	return t_threadId;
}

void* Thread::RunThread(void* args) {
	Thread* th = (Thread*)args;
	t_threadId = GetThreadId();
	th->id_ = t_threadId;
	t_threadName = th->name_;
	pthread_setname_np(pthread_self(), th->name_.c_str());
	th->latch_.countDown();
	th->cb_();
	return static_cast<void*>(0);
}

void Thread::start() {
	if (!running_) {
		running_ = true;
		if (pthread_create(&tid_, NULL, &Thread::RunThread, this)) {	
			running_ = false;
			LOG_ERROR << "Thread start error=" << strerror(errno);
		} else {
			latch_.wait();
		}
	}
}

void Thread::join() {
	if (running_ && !joined_) {
		pthread_join(tid_, NULL);
		joined_  = true;
		running_ = false;
	}
}


} // namespace reyao