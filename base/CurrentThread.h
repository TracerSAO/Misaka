#ifndef MISAKA_CURRENTTHREAD_H
#define MISAKA_CURRENTTHREAD_H

namespace Misaka
{
namespace CurrentThread
{


extern __thread int t_cachedTid;
extern __thread char t_tidString[32];
extern __thread int t_tidStringLength;		// // 5 + 1，多加 1 -> ‘\0
extern __thread const char* t_threadName;
void cachedTid();

inline int tid()
{
	if (__builtin_expect(0 == t_cachedTid, 0))
	{
		cachedTid();
	}
	return t_cachedTid;
}
inline const char* tidString()
{
	return t_tidString;
}
inline int tidStringLength()
{
	return t_tidStringLength;
}
inline const char* threadName()
{
	return t_threadName;
}

bool isMainThread();


}	// namespace CurrentThread
}	// namespace Misaka

#endif // !MISAKA_CURRENTTHREAD_H
