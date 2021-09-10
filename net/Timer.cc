#include "Timer.h"

using namespace Misaka;
using namespace net;

AtomicInt64 Timer::s_numCreated_{};

void Timer::restart(Timestamp now)
{
	if (repeat_)
	{
		expiration_ = addTime(now, interval_);
	}
	else
	{
		expiration_ = Timestamp::invalid();		// ������Ч�� ʱ�������������쳣�Ų�
	}
}