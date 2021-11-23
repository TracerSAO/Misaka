#include "../base/Logging.h"
#include "TimerQueue.h"
#include "TimerId.h"
#include "Timer.h"
#include "EventLoop.h"

#include <sys/timerfd.h>
#include <unistd.h>

#include <iterator>
#include <cassert>

namespace Misaka
{
namespace net
{
namespace detail
{

int createTimerfd()
{
	int fd = ::timerfd_create(CLOCK_MONOTONIC,
		TFD_NONBLOCK | TFD_CLOEXEC);
	return fd;
}

struct timespec howMuchTimeFromNow(Timestamp when)
{
	int64_t microseconds = when.microSecondsSinceEpoch()
		- Timestamp::now().microSecondsSinceEpoch();
	if (100 > microseconds)		// 处理 microseconds 为负的情况，至于为什么不为 0，应该是 muduo 考虑到定时器性能的因素，最好不要短时间再次进入 timer reset
	{
		microseconds = 100;
	}
	struct timespec time;
	time.tv_sec = static_cast<time_t>(
		microseconds / Timestamp::kMicroSecondsPerSecond);
	time.tv_nsec = static_cast<long>(
		(microseconds % Timestamp::kMicroSecondsPerSecond) * 1000);
	return time;
}

void readTimerfd(int timerfd, Timestamp now)
{
	uint64_t howmany;
	ssize_t n = ::read(timerfd, &howmany, sizeof(howmany));
	// assert(n == sizeof howmany);
	if (sizeof(howmany) != n)
	{
		LOG_ERROR << "TimerQueue::handleRead() reads " << n << " bytes instead of 8";
	}
}

void resetTimerfd(int timerfd, Timestamp expiration)
{
	struct itimerspec new_value;
	struct itimerspec old_value;
	bzero(&new_value, sizeof new_value);
	bzero(&old_value, sizeof old_value);
	new_value.it_value = howMuchTimeFromNow(expiration);
	int res = ::timerfd_settime(timerfd, 0, &new_value, &old_value);
	assert(-1 != res);
}

}	// namespace detail
}	// namespace net
}	// namespace Misaka


using namespace Misaka;
using namespace net;
using namespace net::detail;

TimerQueue::TimerQueue(EventLoop* loop):
	loop_(loop),
	timerfd_(createTimerfd()),
	timerfdChannel_(loop, timerfd_)
{
	timerfdChannel_.setReadCallback(
		std::bind(&TimerQueue::handleRead, this));
	timerfdChannel_.enableReading();
}

TimerQueue::~TimerQueue()
{
	timerfdChannel_.disableAll();
	timerfdChannel_.remove();
	::close(timerfd_);

	for (const Entry& timer : timers_)
		delete timer.second;
}

TimerId TimerQueue::addTimer(TimerCallback cb, 
							 Timestamp when, double interval)
{
	Timer* timer = new Timer(cb, when, interval);
	loop_->runInLoop(std::bind(&TimerQueue::addTimerInLoop, this, timer));
	// std::bind(&TimerQueue::addTimerInLoop, this, timer)
	return TimerId(timer, timer->sequence());
	// Q: 为什么会使用 runInLoop() 这安全的跨线程方式，来设置 Timer(定时器) 的添加？
	// A: 向 TimerQueue 中添加 New Timer，是必须修改 TimerQueue 中的用于记录 Timer 的容器
	// 		这样这个 Container 就成会变成竞争资源（因为竞争者不止一位呀 :P )
	//		如果不采用什么保护措施的话，那么 TimerQueue 就是线程不安全的！！！
	//		目前的方案中，我们可使用加锁的方式 OR 目前使用这种安全的跨线程参数传递方式
	//		性能分析：
	//		前者，会使用 “锁”，lock/unlock 并不会造成性能瓶颈，而多个线程对锁的争夺会造成非常严重的性能损耗！！！！
	//			所以这种方案，很显然不过关。现在服务端编程，在能不是用锁的地方都会不去使用锁，而且 muduo 网络库在设计之初，就是要规避掉加锁带来的负面影响；
	//		后者，也是使用到锁，但是这种方案，各个线程对锁的占有时间绝对的短！！！！！！！
	//			这也就是保证了临界区不会很长，临界区越短，锁争用所带来的性能损失越少。
	//			保证方式，就是将其他线程需要注册的函数，作为一种特殊的参数，通过 Linux 内核提供的事件触发文件描述符，
	//			来将操作在借助 IO 多路复用的帮助下，带入到 TimerQueue 所在的线程汇中，实现了 “多线程向单线程” 的转换
	//			最后，只要在这个单线程中，线性的处理容器中所存储的待注册函数即可！
	//			干得漂亮 ο(=•ω＜=)ρ⌒☆
}

void TimerQueue::addTimerInLoop(Timer* timer)
{
	loop_->assertInLoopThread();	// 不支持跨线程
	bool earliestChanged = insert(timer);

	if (earliestChanged)
	{
		resetTimerfd(timerfd_, timer->expiration());
	}
}

bool TimerQueue::insert(Timer* timer)
{
	loop_->assertInLoopThread();
	bool earliestChanged = false;
	Timestamp when = timer->expiration();
	if (timers_.empty() || when < timers_.begin()->first)
	{
		earliestChanged = true;
	}
	timers_.insert(std::move(Entry(when, timer)));
	return earliestChanged;
}

void TimerQueue::handleRead()
{
	loop_->assertInLoopThread();
	Timestamp now = Timestamp::now();
	readTimerfd(timerfd_, now);
	std::vector<Entry> res = getExpired(now);
	for (auto& i : res)
	{
		i.second->run();
	}
	reset(res, now);
}

std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now)
{
	loop_->assertInLoopThread();
	Entry sentry(now, reinterpret_cast<Timer*>(UINTPTR_MAX));	// UINTPTR_MAX 最大指针，确保所有 Timestamp 相同的 timer，一定会被检测出来
	
	std::vector<Entry> expired;
	auto end = timers_.lower_bound(sentry);
	assert(timers_.end() == end || end->first > now);
	std::copy(timers_.begin(), end, std::back_inserter(expired));
	timers_.erase(timers_.begin(), end);

	return expired;
}

void TimerQueue::reset(const std::vector<Entry>& expired, Timestamp now)
{
	loop_->assertInLoopThread();
	for (auto& entry : expired)
	{
		if (entry.second->repeat())
		{
			entry.second->restart(Timestamp::now());
			insert(entry.second);
		}
		else
		{
			delete entry.second;
		}
	}

	Timestamp nextExpire;
	if (!timers_.empty())
	{
		nextExpire = timers_.begin()->first;
	}

	if (nextExpire.valid())
	{
		resetTimerfd(timerfd_, nextExpire);
	}
}
