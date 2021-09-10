#ifndef MISAKA_ATOMIC_H
#define MISAKA_ATOMIC_H

#include "noncopyable.h"
#include <cstdint>

namespace Misaka
{


template <typename T>
class AtomicInteger : public noncopyable
{
public:
	AtomicInteger() :
		value_(0)
	{ }

	T get()
	{
		// >= gcc 4.7
		return __sync_val_compare_and_swap(&value_, 0, 0);
	}
	T getAndAdd(T x)
	{
		return __sync_fetch_and_add(&value_, x);
	}
	T getAndSet(T newValue)
	{
		return __sync_lock_test_and_set(&value_, newValue);
	}

	T addAndGet(T x)
	{
		return getAndAdd(x) + x;
	}
	T incrementAndGet()
	{
		return addAndGet(1);
	}
	T decrementAndGet()
	{
		return addAndGet(-1);
	}
	void increment()
	{
		incrementAndGet();
	}
	void decrement()
	{
		decrement();
	}

private:
	volatile T value_;
};

typedef AtomicInteger<int32_t> AtomicInt32;
typedef AtomicInteger<int64_t> AtomicInt64;


}	// namespace Misaka


#endif // !MISAKA_ATOMIC_H
