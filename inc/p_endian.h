#ifdef _HAVE_P_ENDIAN_H
#define _HAVE_P_ENDIAN_H
#else

#if defined (OS_DARWIN)

#include <libkern/OSByteOrder.h>
#define htobe16(x) OSSwapHostToBigInt16(x)
#define htole16(x) OSSwapHostToLittleInt16(x)
#define be16toh(x) OSSwapBigToHostInt16(x)
#define le16toh(x) OSSwapLittleToHostInt16(x)
#define htobe32(x) OSSwapHostToBigInt32(x)
#define htole32(x) OSSwapHostToLittleInt32(x)
#define be32toh(x) OSSwapBigToHostInt32(x)
#define le32toh(x) OSSwapLittleToHostInt32(x)
#define htobe64(x) OSSwapHostToBigInt64(x)
#define htole64(x) OSSwapHostToLittleInt64(x)
#define be64toh(x) OSSwapBigToHostInt64(x)
#define le64toh(x) OSSwapLittleToHostInt64(x)
#define s6_addr32 __u6_addr.__u6_addr32

#elif defined (OS_FREEBSD) || defined (OS_NETBSD)

#include <sys/endian.h>
#define s6_addr32 __u6_addr.__u6_addr32

#elif defined (OS_LINUX)

#include <endian.h>

#endif
#endif
