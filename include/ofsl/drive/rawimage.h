#ifndef OFSL_DRIVE_RAWIMAGE_H__
#define OFSL_DRIVE_RAWIMAGE_H__

#include <ofsl/drive/drive.h>

#ifdef __cplusplus
extern "C" {
#endif

OFSL_Drive* ofsl_create_rawimage_drive(const char* name, int readonly, size_t sector_size, size_t sector_count);

#ifdef __cplusplus
};
#endif

#endif
