#include "reyao/log.h"
#include "reyao/thread.h"

#include <unistd.h>
#include <sys/time.h>

#include <iostream>
#include <string>

using namespace reyao;

static uint64_t total = 0;

void log_test() {
	g_logger->setFileAppender();
	// g_logger->setFormatter("%d%p%N%t%c%f%l%m%n"); //58
	g_logger->setFormatter("%m%n");
	std::vector<Thread::SPtr> threads;
	struct timeval s, e;
	gettimeofday(&s, nullptr);
	for (int i = 0; i < 5; i++)
	{
		Thread::SPtr t(new Thread([]() {
			for (int i = 0; i < 10000000; i++) {	
				LOG_DEBUG << "hellohellohellohellohellohellohellohellohellohello";
				total += 100;
				// LOG_DEBUG << "helloworld";
				// total += 10;
			}
		}, "writelog_" + std::to_string(i + 1)));

		threads.push_back(t);
		t->start();
	}

	for (auto t : threads)
		t->join();

	gettimeofday(&e, nullptr);
	double sec = (e.tv_sec - s.tv_sec) + (e.tv_usec - s.tv_usec) / 1000000.0;
	double speed = total / sec / 1024 / 1024;
	std::cerr << "time=" << sec << "s " << "total=" << (total / 1024 / 1024) << "mb " << "speed=" << speed << "mb/s" << "\n";
}

int main(int argc, char** argv) {   
	log_test();
	return 0;
}
