#ifndef MISAKA_ENDIANT_H
#define MISAKA_ENDIANT_H

#include <cstdint>
#include <endian.h>

namespace Misaka
{
namespace net
{
namespace sockets
{

// 已验证，g++9 编译时选择 -Wconversion 选项仍会报出 WARNING
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"

inline uint16_t hostToNetwork16(uint16_t host16)
{
	return htobe16(host16);
}

inline uint32_t hostToNetwork32(uint32_t host32)
{
	return htobe32(host32);
}

inline uint64_t hostToNetwork64(uint64_t host64)
{
	return htobe64(host64);
}

inline uint16_t networkToHost16(uint16_t net16)
{
	return be16toh(net16);
}

inline uint32_t networkToHost32(uint32_t net32)
{
	return be32toh(net32);
}

inline uint64_t networkToHost64(uint64_t net64)
{
	return be64toh(net64);
}

#pragma GCC diagnostic pop

}	// namespace sockets
}	// namespace net
}	// namespace Misaka

#endif // !MISAKA_ENDIANT_H
