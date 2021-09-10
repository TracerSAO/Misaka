#ifndef MISAKA_NONCOPYABLE_H
#define MISAKA_NONCOPYABLE_H

namespace Misaka
{

class noncopyable
{
public:
	noncopyable(const noncopyable&) = delete;
	void operator=(const noncopyable&) = delete;
protected:
	noncopyable() = default;
	~noncopyable() = default;
};

}	// namespace Misaka

#endif	// noncopyable.h