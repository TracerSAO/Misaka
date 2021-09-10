#ifndef MISAKA_TIMERID_H
#define MISAKA_TIMERID_H

#include "../base/copyable.h"

namespace Misaka
{
namespace net
{

class Timer;

class TimerId : public copyable
{
public:
	TimerId() :
		timer_(nullptr),
		sequence_(0)
	{
	}
	TimerId(Timer* timer, int64_t sequ) :
		timer_(timer),
		sequence_(sequ)
	{
	}

	friend class TiemrQueue;

private:
	Timer* timer_;
	int64_t sequence_;
};

}	// namespace net
}	// namespace Misaka

#endif // !MISAKA_TIMERID_H
