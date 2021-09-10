#include "../base/Logging.h"
#include "EventLoopThreadPool.h"
#include "EventLoop.h"

using namespace Misaka;
using namespace net;

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseloop,
	const string& name) :
	baseLoop_(CHECKNOTNULL(baseloop)),
	name_(name),
	start_(false),
	numThreads_(0),
	next_(0)
{
}

EventLoopThreadPool::~EventLoopThreadPool()
{
	// Pool 生命周期从属于 main-thread -> TcpServer
	// TcpServer 又是 muduo 网络库运行的核心，main-thread not stop, TcpServer not stop
	// 如果 TcpServer 停止运行了，只可能是被 user 从外部终止，
	// 此时，main-Thread 也必然会终止，
	// main-thread 都停止了，那么 other-son-thread 怎么可能还能继续运行呢？？？
	
	// EventLoopThreadPool dead 是 TcpServer dead 的缩影
	// TcpServer 都死亡了，那 main-thread 必然也会随之结束
	// 既然死亡都是必然的，那就更本没有必要做善后处理了嘛
	// 毕竟，网络端程序，不要求能够正常退出 -> PS: 不都是 CTRL+C 嘛 XP
}

void EventLoopThreadPool::start(const EventLoopThreadPool::ThreadInitCallback& cb)
{
	baseLoop_->assertInLoopThread();
	assert(!started());

	start_ = true;

	for (int i = 0; i < numThreads_; i++)
	{
		char buf[name_.size() + 32];
		snprintf(buf, sizeof buf, "%s%d", name_.c_str(), i);
		EventLoopThread* ptr = new EventLoopThread(cb, buf);
		threads_.push_back(std::unique_ptr<EventLoopThread>(ptr));
		loops_.push_back(ptr->startLoop());
	}
	if (0 == numThreads_ && cb)
	{
		cb(baseLoop_);
	}
}

EventLoop* EventLoopThreadPool::getNextLoop()
{
	baseLoop_->assertInLoopThread();
	assert(started());
	EventLoop* loop = baseLoop_;
	if (!threads_.empty())
	{
		loop = loops_[next_];
		next_++;
		if (implicit_cast<size_t>(next_) >= loops_.size())
		{
			next_ = 0;
		}
	}
	return loop;
}

EventLoop* EventLoopThreadPool::getLoopForHash(size_t hashCode)
{
	baseLoop_->assertInLoopThread();
	assert(started());
	EventLoop* loop = baseLoop_;

	if (!loops_.empty())
	{
		loop = loops_[hashCode % loops_.size()];
	}
	return loop;
}

std::vector<EventLoop*> EventLoopThreadPool::getAllLoops()
{
	baseLoop_->assertInLoopThread();
	assert(started());
	if (loops_.empty())
	{
		return std::vector<EventLoop*>(1, baseLoop_);
	}
	else
	{
		return loops_;
	}
}
