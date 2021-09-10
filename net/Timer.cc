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
		expiration_ = Timestamp::invalid();		// 生成无效的 时间戳，方便后续异常排查
	}
}