#include "ProcessInfo.h"

#include <unistd.h>

#include <cstdio>

using namespace Misaka;

pid_t ProcessInfo::pid()
{
	return ::getpid();
}

string ProcessInfo::pidString()
{
	char buf[32];
	snprintf(buf, sizeof buf, "%d", ::getpid());
	return buf;
}

string ProcessInfo::hostname()
{
	// HOST_NAME_MAX = 64
	// __POSIX__HOST_NAME_MAX = 255
	char buf[256];
	if (::gethostname(buf, sizeof buf) == 0)
	{
		buf[sizeof(buf) - 1] = '\0';
		return buf;
	}
	else
	{
		return string("unknow-hostname");
	}
}