#ifndef OFSL_CRYPTO_CRC32_H__
#define OFSL_CRYPTO_CRC32_H__

#include <stdint.h>
#include <stddef.h>

uint32_t ofsl_gen_crc32(uint32_t crc, const void* buf, size_t len);

#endif
