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
