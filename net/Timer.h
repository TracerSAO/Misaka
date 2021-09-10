#ifndef MISAKA_TIMER_H
#define MISAKA_TIMER_H

#include "../base/noncopyable.h"
#include "../base/Timestamp.h"
#include "../base/Atomic.h"

#include "callbacks.h"

namespace Misaka
{
namespace net
{

class Timer : public noncopyable
{
public:
	Timer(TimerCallback callback, Timestamp timestamp, double interval) :
		callback_(callback),
		expiration_(timestamp),
		interval_(interval),
		repeat_(interval > 0.0),
		sequence_(s_numCreated_.incrementAndGet())
	{ }

	void run() const
	{
		callback_();
	}

	Timestamp expiration() const { return expiration_; }
	bool repeat() const { return repeat_; }
	int64_t sequence() const { return sequence_; }

	void restart(Timestamp now);

	static int64_t numCreated() { return s_numCreated_.get(); }

private:
	const TimerCallback callback_;
	Timestamp expiration_;
	const double interval_;
	const bool repeat_;
	const int64_t sequence_;

	static AtomicInt64 s_numCreated_;
};

}	// net
}	// Misaka


#endif // !MISAKA_TIMER_H
