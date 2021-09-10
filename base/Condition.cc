#include "Condition.h"

#include <cerrno>
#include <cstdint>

bool Misaka::Condition::watiForSeconds(double seconds)
{
	struct timespec abstime;
	::clock_gettime(CLOCK_REALTIME, &abstime);
	const int64_t kNanoSecondsPerSeconds = 1000000000;	// 10^9
	int64_t nanoSeconds = static_cast<int64_t>(seconds * kNanoSecondsPerSeconds);

	abstime.tv_sec += static_cast<time_t>((nanoSeconds + abstime.tv_nsec) / kNanoSecondsPerSeconds);
	abstime.tv_nsec += static_cast<long>((nanoSeconds + abstime.tv_nsec) % kNanoSecondsPerSeconds);

	MutexLock::UnassignedGuard ug(mutex_);
	return ETIMEDOUT == pthread_cond_timedwait(&pcond_, mutex_.getMutexPointer(), &abstime);
}