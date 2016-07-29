/*
 * safequeue test
 * g++ -o test -pthread -D_GLIBCXX_USE_NANOSLEEP -std=c++11 safequeuetest.cpp
 */
#include "safequeue.h"
#include <thread>
#include <iostream>
#include <chrono>

int main(){
	SafeQueue<int, 10> q;
	bool abort = false;
	std::thread t_producer(
	[&q, &abort](){
		std::chrono::seconds duration(1);
		for(int i = 0; i<20; ++i){
			q.push(i);
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
			int obj;
			q.pop(obj);
			std::this_thread::sleep_for(std::chrono::milliseconds(1500));
			std::cout<<"consume a obj: "<<obj<<std::endl;
		}
	});
	t_consumer.join();
	t_producer.join();
	std::cout<<"return!"<<std::endl;
	return 0;
}
