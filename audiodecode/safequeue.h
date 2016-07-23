#ifndef SAFEQUEUE_H
#define SAFEQUEUE_H

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

/*a thread-safe queue*/
template<typename T, int MAXSIZE>
class SafeQueue
{
public:
	void pop(T& data);
	void push(const T& data);
private:
	std::mutex qMutex;
	std::queue<T> q;
	std::condition_variable condNotFull;
	std::condition_variable condNotEmpty;
};

template<typename T, int MAXSIZE>
void SafeQueue<T, MAXSIZE>::push(const T& data)
{
	std::unique_lock<std::mutex> lock(qMutex);
	/*if queue was full, wait */
	if(q.size() >= MAXSIZE){
		condNotFull.wait(lock, [this]{return q.size() < MAXSIZE;});
	}
	q.push(data);
	condNotEmpty.notify_one();
}

template<typename T, int MAXSIZE>
void SafeQueue<T, MAXSIZE>::pop(T& data)
{
	std::unique_lock<std::mutex> lock(qMutex);
	/*if queue was empty, wait*/
	if(q.empty()){
		condNotEmpty.wait(lock, [this]{return !q.empty();});
	}
	data = q.front();
	q.pop();
	condNotFull.notify_one();
}

#endif