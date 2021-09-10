#ifndef MISAKA_COUNTDOWNLATCH_H
#define MISAKA_COUNTDOWNLATCH_H

#include "Condition.h"

namespace Misaka
{

class CountDownLatch : noncopyable
{
public:
	CountDownLatch(int countDown);

	void countDown();

	void wait();

	int getCount() const;

private:
	mutable MutexLock mutex_;
	Condition cond_;

	int countDown_;
};

}	// namespace Misaka

#endif // !MISAKA_COUNTDOWNLATCH_H
