#include "reyao/thread.h"
#include "reyao/log.h"

#include <unistd.h>
#include <sys/syscall.h>

namespace reyao {

thread_local std::string t_thread_name = "MainThread";
thread_local pid_t t_thread_id = 0;


Thread::Thread(ThreadFunc cb, const std::string& name)
	: tid_(0),
	  id_(0),
	  cb_(cb),
	  name_(name),
	  latch_(1),
	  started_(false),
	  joined_(false) {

}

Thread::~Thread() {
	if (started_ && !joined_) {
		pthread_detach(tid_);
	}
}

void Thread::SetThreadName(const std::string& name) {
	t_thread_name = name;
}

const std::string& Thread::GetThreadName() {
	return t_thread_name;
}

pid_t Thread::GetThreadId() {
	if (t_thread_id == 0) {
		t_thread_id = syscall(SYS_gettid);
	}
	return t_thread_id;
}

void* Thread::RunThread(void* args) {
	Thread* th = (Thread*)args;
	t_thread_id = GetThreadId();
	th->id_ = t_thread_id;
	t_thread_name = th->name_;
	pthread_setname_np(pthread_self(), th->name_.c_str()); //FIXME:
	th->latch_.countDown();
	th->cb_();
	return static_cast<void*>(0);
}

void Thread::start() {
	if (!started_) {
		started_ = true;
		if (pthread_create(&tid_, NULL, &Thread::RunThread, this)) {	
			started_ = false;
			LOG_ERROR << "Thread start error=" << strerror(errno);
		} else {
			latch_.wait(); //等待线程启动再返回
		}
	}
}

void Thread::join() {
	if (started_ && !joined_) {
		pthread_join(tid_, NULL);
		joined_  = true;
		started_ = false;
	}
}


} //namespace reyao