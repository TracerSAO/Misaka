#include "../../net/EventLoopThread.h"
#include "../../net/EventLoop.h"
#include "../../net/TimerId.h"
#include <unistd.h>

using namespace Misaka;
using namespace net;

class Plante {
public:
    void foo(EventLoop* loop) {
        loop->runEvery(3, std::bind(&Plante::funcA, this));
        loop->runEvery(1, std::bind(&Plante::funcB, this));
    }

    void funcA() {
        printf(">>> funcA()\n");
    }
    void funcB() {
        printf(">>> funcB()\n");
    }
};

class Printer {
public:
	Printer() : 
		count_(0)
    { }
    void func(EventLoop* loop_)
	{
		loop_->runEvery(1, std::bind(&Printer::print, this));
		loop_->runEvery(2, std::bind(&Printer::print_iloveAsuna, this));
	}

	void print()
	{
		printf("bilibili~ happy 11 anniversary:) - ");
		MutexLockGuard lock(mutex_);
		count_ += 1;
		printf("%d\n", count_);
	}
	void print_iloveAsuna()
	{
		printf("i love Asuna - ");
		MutexLockGuard lock(mutex_);
		count_ += 2;
		printf("%d\n", count_);
	}

private:
	int count_;
	MutexLock mutex_;
};

int main()
{
    Plante obj;
    std::unique_ptr<Printer> printer;
    
	printer.reset(new Printer());
    EventLoopThread eventloopThread(std::bind(
			&Printer::func, 
			printer.get(), 
			std::placeholders::_1), 
		"PlanteThread");
	obj.foo(eventloopThread.startLoop());
	
	sleep(10);
}