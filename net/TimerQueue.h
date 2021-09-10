#ifndef MISAKA_TIMERQUEUE_H
#define MISAKA_TIMERQUEUE_H

#include <set>
#include <vector>

#include "../base/noncopyable.h"
#include "../base/Timestamp.h"
#include "callbacks.h"
#include "Channel.h"

namespace Misaka
{
namespace net
{

class EventLoop;
class Timer;
class TimerId;

class TimerQueue : public noncopyable
{
public:
	explicit TimerQueue(EventLoop* loop);
	~TimerQueue();

	TimerId addTimer(TimerCallback cb, Timestamp when, double interval);

private:
	typedef std::pair<Timestamp, Timer*> Entry;
	typedef std::set<Entry> TimerList;

	void addTimerInLoop(Timer* timer);
	void handleRead();
	std::vector<Entry> getExpired(Timestamp now);
	void reset(const std::vector<Entry>& expired, Timestamp now);

	bool insert(Timer* timer);

private:
	EventLoop* loop_;
	const int timerfd_;
	Channel timerfdChannel_;

	TimerList timers_;
};

}	// namespace net
}	// namespace Misaka

#endif // !MISAKA_TIMERQUEUE_H
