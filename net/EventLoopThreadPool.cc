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
	// Pool �������ڴ����� main-thread -> TcpServer
	// TcpServer ���� muduo ��������еĺ��ģ�main-thread not stop, TcpServer not stop
	// ��� TcpServer ֹͣ�����ˣ�ֻ�����Ǳ� user ���ⲿ��ֹ��
	// ��ʱ��main-Thread Ҳ��Ȼ����ֹ��
	// main-thread ��ֹͣ�ˣ���ô other-son-thread ��ô���ܻ��ܼ��������أ�����
	
	// EventLoopThreadPool dead �� TcpServer dead ����Ӱ
	// TcpServer �������ˣ��� main-thread ��ȻҲ����֮����
	// ��Ȼ�������Ǳ�Ȼ�ģ��Ǿ͸���û�б�Ҫ���ƺ�������
	// �Ͼ�������˳��򣬲�Ҫ���ܹ������˳� -> PS: ������ CTRL+C �� XP
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
