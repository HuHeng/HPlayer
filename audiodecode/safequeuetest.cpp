/*
 * safequeue test
 * g++ -o test -pthread -D_GLIBCXX_USE_NANOSLEEP -std=c++11 safequeuetest.cpp
 */
#include "safequeue.h"
#include <thread>
#include <iostream>
#include <chrono>
#include <memory>

struct test
{
	int count;
	test(int i):count(i){}
};

typedef std::shared_ptr<test> ptest;

int main(){
	SafeQueue<ptest, 10> q;
	bool abort = false;
	std::thread t_producer(
	[&q, &abort](){
		std::chrono::seconds duration(1);
		for(int i = 0; i<20; ++i){
			q.push(std::make_shared<test>(i));
			std::cout<<"produce i: "<<i<<std::endl;
			std::this_thread::sleep_for(duration);
		}
		abort = true;
		std::cout<<"abort: "<<abort<<std::endl;
	});
	std::thread t_consumer(
	[&q, &abort](){
		for(;;){
			if(abort)
				break;
			ptest obj;
			q.pop(obj);
			std::this_thread::sleep_for(std::chrono::milliseconds(1500));
			std::cout<<"consume a obj: "<<obj->count<<std::endl;
		}
	});
	t_consumer.join();
	t_producer.join();
	std::cout<<"return!"<<std::endl;
	return 0;
}
