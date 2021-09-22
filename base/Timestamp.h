#ifndef MISAKA_TIMESTAMP_H
#define MISAKA_TIMESTAMP_H

#include "copyable.h"
#include "Types.h"

#include <boost/operators.hpp>

/**
 * boost::equality_comparable<>  && boost::less_than_comparable<>
 * 
 * since C++ 20，才被引入 cpp::std
 */

namespace Misaka
{

using std::string;
class Timestamp : public copyable,
				  public boost::equality_comparable<Timestamp>,
				  public boost::less_than_comparable<Timestamp>
{
public:
	Timestamp() :
		microSecondsSinceEpoch_(0)
	{ }
	explicit Timestamp(int64_t microSecondSinceEpochArg) :
		microSecondsSinceEpoch_(microSecondSinceEpochArg)
	{ }

	void swap(Timestamp& that)
	{
		std::swap(microSecondsSinceEpoch_, that.microSecondsSinceEpoch_);
	}

	string toString() const;
	string toFormattedString(bool showMicroseconds = true) const;

	bool valid() const { return microSecondsSinceEpoch_ > 0; }

	int64_t microSecondsSinceEpoch() const { return microSecondsSinceEpoch_; }
	time_t secondSinceEpoch() const
	{
		return static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond);
	}

	static Timestamp now();
	static Timestamp invalid()		// return invalid Timestamp, why? maybe somebody thinked is speically
	{
		return Timestamp();
	}

	static Timestamp fromUnixTime(time_t t)
	{
		return fromUnixTime(t, 0);
	}

	static Timestamp fromUnixTime(time_t t, int microseconds)
	{
		return Timestamp(static_cast<int64_t>(t) * kMicroSecondsPerSecond + microseconds);
	}

	static const int kMicroSecondsPerSecond = 1000 * 1000;
private:
	int64_t microSecondsSinceEpoch_;
	// 292,471.20867753601623541349568747
	// int64_t 可以表示上下 30 万年的时间限 (正负表上下)
};

inline bool operator==(const Timestamp& lt, const Timestamp& rt)
{
	if (lt.microSecondsSinceEpoch() == rt.microSecondsSinceEpoch())
		return true;
	return false;
}
inline bool operator<(const Timestamp& lt, const Timestamp& rt)
{
	if (lt.microSecondsSinceEpoch() < rt.microSecondsSinceEpoch())
		return true;
	return false;
}

inline double timeDifference(Timestamp hight, Timestamp low)
{
	int64_t diff = hight.microSecondsSinceEpoch() - low.microSecondsSinceEpoch();
	return static_cast<double>(diff) / Timestamp::kMicroSecondsPerSecond;
}

inline Timestamp addTime(Timestamp timestamp, double seconds)
{
	int64_t delta = static_cast<int64_t>(seconds * Timestamp::kMicroSecondsPerSecond);
	return Timestamp(timestamp.microSecondsSinceEpoch() + delta);
}


}	// Misaka

#endif // !TIMESTAMP_H