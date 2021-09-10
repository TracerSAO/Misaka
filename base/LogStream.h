#ifndef MISAKA_LOGSTREAM_H
#define MISAKA_LOGSTREAM_H

#include "noncopyable.h"
#include "StringPiece.h"
#include "Types.h"

#include <cassert>
#include <cstring>	// memcpy

namespace Misaka
{

namespace detail
{

const int kSmallBuffer = 4000;
const int kLargeBuffer = 4000 * 1000;

template <int SIZE>
class FixedBuffer : public noncopyable
{
public:
	FixedBuffer():
		cur_(data_)
	{
		setCookie(cookieStart);
	}
	~FixedBuffer()
	{
		setCookie(cookieEnd);
	}

	void append(const char* buf, size_t len)
	{
		if (implicit_cast<size_t>(avail()) >= len)
		{
			memcpy(cur_, buf, len);
			cur_ += len;
		}
	}
			
	const char* data() const { return data_; }
	int length() const { return static_cast<int>(cur_ - data_); }

	char* current() { return cur_; }
	int avail() const { return static_cast<int>(end() - cur_); }
	void add(size_t len) { cur_ += len; }	// 配合特殊的 char write to buffer 使用，不同于 memcpy 的整块写入

	void reset() { cur_ = data_; }
	void bzero() { ::bzero(data_, sizeof data_); }

	// for GDB
	const char* debugString();
	// 可以定位那些在程序崩溃后，尚未来得及写入 DISK 的 LOG-data
	void setCookie(void (*cookie)()) { cookie_ = cookie; }

	// for unit-test || 对我而言，方便输出查看结果
	string toString() const { return string(data_, length()); }
	StringPiece toStringPiece() const { return StringPiece(data_, length()); }

private:
	const char* end() const { return data_ + sizeof data_; }
			
	static void cookieStart();
	static void cookieEnd();

private:
	void (*cookie_)();
	char data_[SIZE];
	char* cur_;
};

}	// namespace detial

class LogStream : public noncopyable
{
	using self = LogStream;
public:
	using Buffer = detail::FixedBuffer<detail::kSmallBuffer>;

	self& operator<<(bool v) {
		buffer_.append( (v ? "1" : "0"), 1);
		return *this;
	}

	self& operator<<(short);
	self& operator<<(unsigned short);
	self& operator<<(int);
	self& operator<<(unsigned int);
	self& operator<<(long);
	self& operator<<(unsigned long);
	self& operator<<(long long);
	self& operator<<(unsigned long long);

	self& operator<<(const void*);
	
	self& operator<<(float v) {
		*this << static_cast<double>(v);
		return *this;
	}
	self& operator<<(double);

	self& operator<<(char v) {
		buffer_.append(&v, 1);
		return *this;
	}

	self& operator<<(const char* str) {
		if (str)
		{
			buffer_.append(str, strlen(str));
		}
		else
		{
			buffer_.append("(null)", 6);
		}
		return *this;
	}

	self& operator<<(const unsigned char* str) {
		return *this << reinterpret_cast<const char*>(str);
	}

	self& operator<<(const string& v) {
		buffer_.append(v.c_str(), v.length());
		return *this;
	}

	self& operator<<(const StringPiece& v) {
		buffer_.append(v.data(), v.size());
		return *this;
	}

	self& operator<<(const Buffer& v) {
		buffer_.append(v.data(), v.length());
		return *this;
	}

	void append(const char* data, int len) { buffer_.append(data, len); }
	const Buffer& buffer() const { return buffer_; }
	void resetBuffer() { buffer_.reset(); }

private:
	void staticCheck();

	template <typename T>
	void formatInteget(T);

private:
	Buffer buffer_;
	static const int kMaxNumbericSize = 32;		// 用于 内置数据类型 转换到 string
};

class Fmt
{
public:
	template <typename T>
	Fmt(const char* str, T val);

	const char* data() const { return buf_; }
	int length() const { return length_; }

private:
	char buf_[32];
	int length_;
};

// 不知道是不是 muduo 的疏忽，logging.cc 中并没有服用这个接口，而是使用新的接口
// LogStream& operator<<(Logstream& os, const T& val);
// 个人认为两者并没有什么太大区别，相反，前者还会多一次 ctor
// 对外提供一个用户可以直接将 class Fmt 输出到 LogStream 的接口
inline LogStream& operator<<(LogStream& os, const Fmt& us)
{
	os.append(us.data(), us.length());
	return os;
}

}	// namespace Misaka

#endif // !MISAKA_LOGSTREAM_H