#ifndef MISAKA_TYPES_H
#define MISAKA_TYPES_H

#include <string>
#include <cstring>
#include <cstdint>

namespace Misaka
{

using std::string;

template <typename To, typename From>
inline To implicit_cast(From const &f)
{
	return f;
}

}	// namespace Misaka

#endif // !MISAKA_TYPES_H
