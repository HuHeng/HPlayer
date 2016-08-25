#include <iostream>
#include <thread>
using namespace std;


class ThreadObj
{
public:
	virtual void run(){cout<<"aa"<<endl;}
	void start(){
		pThread = new std::thread(std::mem_fn(&ThreadObj::run), this);
	}
	void join(){
		pThread->join();
		delete pThread;
	}
private:
	std::thread* pThread;
};

class t1 : public ThreadObj
{
public:
	virtual void run(){cout<<"t1 thread"<<endl;}
};

class t2 : public ThreadObj
{
public:
	virtual void run(){cout<<"t2 thread"<<endl;}
};

int main()
{
	ThreadObj* t_1 = new t1;
	ThreadObj* t_2 = new t2;
	t_1->start();
	t_2->start();
	t_1->join();
	t_2->join();
}
