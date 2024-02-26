#include "crypto/crc32.h"

#include <zlib.h>

#include "config.h"

uint32_t ofsl_gen_crc32(uint32_t crc, const void* buf, size_t len)
{
#ifdef USE_ZLIB
    return crc32(crc, buf, len);
#else
#error CRC32 unimplemented
#endif
}
