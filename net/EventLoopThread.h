#ifndef MISAKA_EVENTLOOPTHREAD_H
#define MISAKA_EVENTLOOPTHREAD_H

#include "../base/Thread.h"

namespace Misaka
{
namespace net
{

class EventLoop;

class EventLoopThread {
public:
	typedef std::function<void (EventLoop*)> ThreadInitialCallback;

public:
	EventLoopThread(ThreadInitialCallback cb = ThreadInitialCallback(),
					const string& name = string());
	~EventLoopThread();
	EventLoop* startLoop();

private:
	void threadFunc();

private:
	bool exiting_;
	MutexLock mutex_;
	Condition cond_;
	Thread thread_;
	EventLoop* loop_;
	ThreadInitialCallback callback_;
};

}	// namespace net
}	// namespace Misaka

#endif // !MISAKA_EVENTLOOPTHREAD_H
