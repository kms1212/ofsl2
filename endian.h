#ifndef ENDIAN_H__
#define ENDIAN_H__

#include <stdint.h>

#include "config.h"

static inline uint16_t byteswap16(uint16_t val)
{
    return
        ((val & 0xFF00) >> 8) |
        ((val & 0x00FF) << 8);
}

static inline uint32_t byteswap32(uint32_t val)
{
    return
        ((val & 0xFF000000) >> 24) |
        ((val & 0x00FF0000) >>  8) |
        ((val & 0x0000FF00) <<  8) |
        ((val & 0x000000FF) << 24);
}

static inline uint64_t byteswap64(uint64_t val)
{
    return
        ((val & 0xFF00000000000000) >> 56) |
        ((val & 0x00FF000000000000) >> 40) |
        ((val & 0x0000FF0000000000) >> 24) |
        ((val & 0x000000FF00000000) >>  8) |
        ((val & 0x00000000FF000000) <<  8) |
        ((val & 0x0000000000FF0000) << 24) |
        ((val & 0x000000000000FF00) << 40) |
        ((val & 0x00000000000000FF) << 56);
}

#ifdef BYTE_ORDER_BIG_ENDIAN
#define htole16(v) byteswap16(v)
#define htobe16(v) (v)
#define htole32(v) byteswap32(v)
#define htobe32(v) (v)
#define htole64(v) byteswap64(v)
#define htobe64(v) (v)

#else
#define htole16(v) (v)
#define htobe16(v) byteswap16(v)
#define htole32(v) (v)
#define htobe32(v) byteswap32(v)
#define htole64(v) (v)
#define htobe64(v) byteswap64(v)

#endif

#endif
