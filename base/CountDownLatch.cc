#include "CountDownLatch.h"

using namespace Misaka;

CountDownLatch::CountDownLatch(int countDown) :
	mutex_(),
	cond_(mutex_),
	countDown_(countDown)
{
}

void CountDownLatch::countDown()
{
	MutexLockGuard lock(mutex_);
	--countDown_;
	if (0 == countDown_)
	{
		cond_.notifyAll();
	}
}

void CountDownLatch::wait()
{
	MutexLockGuard lock(mutex_);
	while (0 < countDown_)
		cond_.wait();
}

int CountDownLatch::getCount() const
{
	MutexLockGuard lock(mutex_);
	return countDown_;
}