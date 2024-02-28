#ifndef CRYPTO_CRC32_H__
#define CRYPTO_CRC32_H__

#include <stdint.h>
#include <stddef.h>

uint32_t _gen_crc32(uint32_t crc, const void* buf, size_t len);

#endif
