#ifndef MISAKA_CONDITION_H
#define MISAKA_CONDITION_H

#include "Mutex.h"

namespace Misaka
{

class Condition : noncopyable
{
public:
	Condition(MutexLock& mutex) :
		pcond_(PTHREAD_COND_INITIALIZER),
		mutex_(mutex)
	{
	}
	~Condition()
	{
		pthread_cond_destroy(&pcond_);
	}

	void wait()
	{
		MutexLock::UnassignedGuard UG(mutex_);
		pthread_cond_wait(&pcond_, mutex_.getMutexPointer());
	}
	bool watiForSeconds(double seconds);

	void notify()
	{
		pthread_cond_signal(&pcond_);
	}

	void notifyAll()
	{
		pthread_cond_broadcast(&pcond_);
	}

private:
	pthread_cond_t pcond_;
	MutexLock& mutex_;
};

}	// namespace Misaka

#endif // !#define MISAKA_CONDITION_H
