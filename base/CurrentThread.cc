#include "CurrentThread.h"

#include <sys/syscall.h>
#include <unistd.h>

#include <type_traits>

namespace Misaka
{
namespace CurrentThread
{


__thread int t_cachedTid = 0;
__thread char t_tidString[32];
__thread int t_tidStringLength = 6;		// // 5 + 1，多加 1 -> ‘\0
__thread const char* t_threadName = "unknown";
static_assert(std::is_same<int, pid_t>::value, "pid_t should be int");


}	// namespace CurrentThread
}	// namespace Misaka


