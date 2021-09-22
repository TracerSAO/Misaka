#include "LogStream.h"

#include <algorithm>
#include <limits>
#include <type_traits>

#include <cassert>
#include <cstring>
#include <cstdint>
#include <cstdio>


#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS		// 定义后才能使用 PRI64 format type
#endif // !__STDC_FORMAT_MACROS

#include <inttypes.h>

using namespace Misaka;
using namespace detail;

#pragma GCC diagnostic ignored "-Wtype-limits"

namespace Misaka
{
namespace detail
{

	const char digits[] = "9876543210123456789";
	const char* zero = digits + 9;
	static_assert(sizeof(digits) == 20, "wrong number of digits");

	const char digitsHex[] = "0123456789ABCDEF";
	static_assert(sizeof(digitsHex) == 17, "wrong number of digitsHex");

	// 高效的 int 转换为 string 算法，by Matthew Wilson.
	template<typename T>
	size_t convert(char* buf, T val)
	{
		T temp = val;
		char* ptr = buf;
		do
		{
			int index = static_cast<int>(temp % 10);
			*ptr++ = zero[index];	// index 可正可负 -> zero[-1] = zero + -1 || zero[0] = zero + 0
			temp /= 10;
		} while (0 != temp);
		
		if (0 > val)
			*ptr++ = '-';
		*ptr = '\0';
		std::reverse(buf, ptr);		// range: [buf, ptr)，前闭后开

		return ptr - buf;			// 返回的值元素的个数，不包含 '\0'，原因是 cur_ 不会指向 '\0'
	}

	size_t convertHex(char* buf, uintptr_t val)
	{
		uintptr_t temp = val;
		char* ptr = buf;
		do
		{
			int index = static_cast<int>(temp % 16);
			*ptr++ = digitsHex[index];
			temp /= 16;
		} while (0 != temp);
		
		*ptr = '\0';
		std::reverse(buf, ptr);

		return ptr - buf;
	}

	template class FixedBuffer<kSmallBuffer>;
	template class FixedBuffer<kLargeBuffer>;

}	// namespace detail
}	// namespace Misaka

template <int size>
const char* FixedBuffer<size>::debugString()
{
	*cur_ = '\0';
	return data_;
}

template <int size>
void FixedBuffer<size>::cookieStart()
{
}

template <int size>
void FixedBuffer<size>::cookieEnd()
{
}


// Q: 为什么 kMaxNumbericSize - 10 ？？？
void LogStream::staticCheck()
{
	static_assert(kMaxNumbericSize - 10 > std::numeric_limits<double>::digits10,
		"kMaxNumbericSize isn't larget");
	static_assert(kMaxNumbericSize - 10 > std::numeric_limits<long double>::digits10,
		"kMaxNumbericSize isn't larget");
	static_assert(kMaxNumbericSize - 10 > std::numeric_limits<long>::digits10,
		"kMaxNumbericSize isn't larget");
	static_assert(kMaxNumbericSize - 10 > std::numeric_limits<long long>::digits10,
		"kMaxNumbericSize isn't larget");
}

template <typename T>
void LogStream::formatInteger(T val)
{
	if (buffer_.avail() >= kMaxNumbericSize)
	{
		size_t len = convert(buffer_.current(), val);
		buffer_.add(len);
	}
}

LogStream& LogStream::operator<<(short val)
{
	return *this << static_cast<int>(val);
}

LogStream& LogStream::operator<<(unsigned short val)
{
	return *this << static_cast<int>(val);
}

LogStream& LogStream::operator<<(int val)
{
	formatInteger(val);
	return *this;
}
LogStream& LogStream::operator<<(unsigned int val)
{
	formatInteger(val);
	return *this;
}
LogStream& LogStream::operator<<(long val)
{
	formatInteger(val);
	return *this;
}
LogStream& LogStream::operator<<(unsigned long val)
{
	formatInteger(val);
	return *this;
}
LogStream& LogStream::operator<<(long long val)
{
	formatInteger(val);
	return *this;
}
LogStream& LogStream::operator<<(unsigned long long val)
{
	formatInteger(val);
	return *this;
}

LogStream& LogStream::operator<<(const void* val)
{
	uintptr_t v = reinterpret_cast<uintptr_t>(val);
	if (buffer_.avail() >= kMaxNumbericSize)
	{
		char* buf = buffer_.current();
		buf[0] = '0';
		buf[1] = 'x';
		size_t len = convertHex(buf + 2, v);
		buffer_.add(len + 2);
	}
	return *this;
}

LogStream& LogStream::operator<<(double val)
{
	if (buffer_.avail() >= kMaxNumbericSize)
	{
		int len = snprintf(buffer_.current(), kMaxNumbericSize, "%.12g", val);
		buffer_.add(len);
	}
	return *this;
}

template <typename T>
Fmt::Fmt(const char* fmt, T val)
{
	static_assert(std::is_arithmetic<T>::value == true, "Must be arithmetic type");
	length_ = snprintf(buf_, sizeof buf_, fmt, val);
	assert(static_cast<size_t>(length_) < sizeof buf_);	// 强校验？？？
}


// instance template object

template Fmt::Fmt(const char* fmt, char);

template Fmt::Fmt(const char* fmt, short);
template Fmt::Fmt(const char* fmt, unsigned short);
template Fmt::Fmt(const char* fmt, int);
template Fmt::Fmt(const char* fmt, unsigned int);
template Fmt::Fmt(const char* fmt, long);
template Fmt::Fmt(const char* fmt, unsigned long);
template Fmt::Fmt(const char* fmt, long long);
template Fmt::Fmt(const char* fmt, unsigned long long);

template Fmt::Fmt(const char* fmt, float);
template Fmt::Fmt(const char* fmt, double);