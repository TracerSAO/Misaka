#ifndef MISAKA_EVENTLOOPTHREADPOOL_H
#define MISAKA_EVENTLOOPTHREADPOOL_H

#include "../base/noncopyable.h"
#include "../base/Mutex.h"
#include "../base/Condition.h"
#include "EventLoopThread.h"

#include <vector>
#include <memory>
#include <functional>

namespace Misaka
{
namespace net
{

class EventLoop;

class EventLoopThreadPool : noncopyable
{
public:
	typedef std::function<void (EventLoop*)> ThreadInitCallback;
public:
	EventLoopThreadPool(EventLoop*, const string&);
	~EventLoopThreadPool();

	void setThreadNum(int numThreads) { numThreads_ = numThreads; }
	void start(const ThreadInitCallback& cb = ThreadInitCallback());

	EventLoop* getNextLoop();

	EventLoop* getLoopForHash(size_t hashCode);

	std::vector<EventLoop*> getAllLoops();

	bool started() const { return start_; }
	const string& name() const { return name_; }

private:
	EventLoop* baseLoop_;
	string name_;
	bool start_;
	int numThreads_;
	int next_;
	std::vector<std::unique_ptr<EventLoopThread>> threads_;
	std::vector<EventLoop*> loops_;
};

}	// namespace net
}	// namespace Misaka

#endif // !MISAKA_EVENTLOOPTHREADPOOL_H
