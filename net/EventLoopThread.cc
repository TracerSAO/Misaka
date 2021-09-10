#include "EventLoopThread.h"
#include "EventLoop.h"

using namespace Misaka;
using namespace net;

EventLoopThread::EventLoopThread(ThreadInitialCallback cb,
								const string& name):
	exiting_(false),
	mutex_(),
	cond_(mutex_),
	thread_(std::bind(&EventLoopThread::threadFunc, this), name),
	loop_(nullptr),
	callback_(cb)
{
}

EventLoopThread::~EventLoopThread()
{
	exiting_ = true;
	if (nullptr != loop_)
	{
		loop_->quit();
		thread_.join();
	}
}

EventLoop* EventLoopThread::startLoop()
{
	assert(!thread_.started());
	thread_.start();

	EventLoop* loop = nullptr;
	{
		MutexLockGuard lock(mutex_);	// ͬ other thread ִ�е� threadFunc ���� mutex_ ����� ���� ��ζ�� loop_ �Ŀ���Ȩs
		while (nullptr == loop_)
			cond_.wait();
		loop = loop_;
	}
	return loop;
}

void EventLoopThread::threadFunc()
{
	EventLoop loop;
	
	if (callback_)
	{
		callback_(&loop);
	}

	{
		MutexLockGuard lock(mutex_);
		loop_ = &loop;
		cond_.notify();
	}

	loop.loop();
	MutexLockGuard lock(mutex_);
	loop_ = nullptr;
}