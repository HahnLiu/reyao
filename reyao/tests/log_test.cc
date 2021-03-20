#include "reyao/log.h"
#include "reyao/thread.h"
#include "reyao/scheduler.h"
#include "reyao/mutex.h"

#include <unistd.h>
#include <sys/time.h>

#include <iostream>
#include <string>

using namespace reyao;

static int cnt = 5;
Mutex mtx;

void log_test(int t, int r) {
	qlog::QLog::GetInstance()->SetAppender("file");
	qlog::LogFormatter::SetFormat("%time ");
	std::vector<Thread::SPtr> threads;
	struct timeval s, e;
	gettimeofday(&s, nullptr);
	for (int i = 0; i < t; i++)
	{
		Thread::SPtr t(new Thread([&]() {
			std::string msg(100, 'a');
			for (int i = 0; i < r; i++) {	
				LOG_DEBUG << "hellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohello";
			}
		}, "writelog_" + std::to_string(i + 1)));

		threads.push_back(t);
		t->start();
	}

	for (auto t : threads)
		t->join();

	int64_t total = qlog::QLog::GetInstance()->getSinkBytes();
	gettimeofday(&e, nullptr);
	double sec = (e.tv_sec - s.tv_sec) + (e.tv_usec - s.tv_usec) / 1000000.0;
	double speed = total / sec / 1024 / 1024;
	std::cerr << "time=" << sec << "s " << "total=" << (total / 1024 / 1024) << "mb " << "speed=" << speed << "mb/s" << "\n";
	std::cerr << "req_per_time:" << (t * r) / sec << " req/sec" << std::endl;
}

void logTestInCo(int r) {
	for (int i = 0; i < r; i++)
	{
		LOG_FATAL << "hellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohellohello";
	}
	{	
		MutexGuard lock(mtx);
		cnt--;
		std::cerr << cnt << std::endl;
		if (cnt == 0) {
			Worker::GetScheduler()->stop();
		}
	}
	
}

int main(int argc, char** argv) {   
	int t = atoi(argv[1]);
	int r = atoi(argv[2]);
	qlog::QLog::GetInstance()->SetAppender("file");
	qlog::LogFormatter::SetFormat("%time ");
	qlog::QLog::SetLevel(qlog::LogLevel::FATAL);
	Scheduler sche(t);
	sche.startAsync();
	struct timeval s, e;
	gettimeofday(&s, nullptr);
	for (int i = 0; i < t; i++)
		sche.addTask(std::bind(logTestInCo, r));
	
	sche.wait();
	gettimeofday(&e, nullptr);
	int64_t total = qlog::QLog::GetInstance()->getSinkBytes();
	double sec = (e.tv_sec - s.tv_sec) + (e.tv_usec - s.tv_usec) / 1000000.0;
	double speed = total / sec / 1024 / 1024;
	std::cerr << "time=" << sec << "s " << "total=" << (total / 1024 / 1024) << "mb " << "speed=" << speed << "mb/s" << "\n";
	std::cerr << "req_per_time:" << (t * r) / sec << " req/sec" << std::endl;
}
