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
	SafeQueue():abortRequest(0){}
    bool pop(T& data);
    bool push(const T& data);
    void clear();
	void abort();
	unsigned int size();
    T& front();
private:
	std::mutex qMutex;
	std::queue<T> q;
	std::condition_variable condNotFull;
	std::condition_variable condNotEmpty;
	int abortRequest;
};

template<typename T, int MAXSIZE>
bool SafeQueue<T, MAXSIZE>::push(const T& data)
{
	std::unique_lock<std::mutex> lock(qMutex);
	if(abortRequest == 1){
        return false;
	}
	/*if queue was full, wait */
	if(q.size() >= MAXSIZE){
		condNotFull.wait(lock, [this]{return q.size() < MAXSIZE || abortRequest;});
	}
    if(abortRequest)
        return false;
	q.push(data);
	condNotEmpty.notify_one();
    return true;
}

template<typename T, int MAXSIZE>
bool SafeQueue<T, MAXSIZE>::pop(T& data)
{
	std::unique_lock<std::mutex> lock(qMutex);
	if(abortRequest == 1){
        return false;
	}
	/*if queue was empty, wait*/
	if(q.empty()){
		condNotEmpty.wait(lock, [this]{return !q.empty() || abortRequest;});
	}
    if(abortRequest)
        return false;
	data = q.front();
	q.pop();
	condNotFull.notify_one();
    return true;
}

template<typename T, int MAXSIZE>
void SafeQueue<T, MAXSIZE>::clear()
{
    std::unique_lock<std::mutex> lock(qMutex);
    q.swap(SafeQueue<T, MAXSIZE>());
}

template<typename T, int MAXSIZE>
void SafeQueue<T, MAXSIZE>::abort()
{
	/*set abortRequest and send signal*/
	std::unique_lock<std::mutex> lock(qMutex);
	abortRequest = 1;
	condNotFull.notify_all();
	condNotEmpty.notify_all();
}

template<typename T, int MAXSIZE>
unsigned int SafeQueue<T, MAXSIZE>::size()
{
	std::unique_lock<std::mutex> lock(qMutex);
	return q.size();
}

template<typename T, int MAXSIZE>
T& SafeQueue<T, MAXSIZE>::front()
{
    return q.front();
}

#endif
