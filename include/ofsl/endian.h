#ifndef OFSL_ENDIAN_H__
#define OFSL_ENDIAN_H__

#define byteswap16(v)   (((v & 0xFF00) >> 8) | ((v & 0x00FF) << 8))
#define byteswap32(v)   (((v & 0xFF000000) >> 24) | ((v & 0x00FF0000) >> 8) | ((v & 0x0000FF00) << 8) | ((v & 0x000000FF) << 24))

#endif
