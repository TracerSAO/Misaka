#ifndef MISAKA_PROCESSINFO_H
#define MISAKA_PROCESSINFO_H

#include "Types.h"

namespace Misaka
{
namespace ProcessInfo
{

	pid_t pid();
	string pidString();
	string hostname();

}	// namespace ProcessInfo
}	// namespace Misaka

#endif // !MISAKA_PROCESSINFO_H
