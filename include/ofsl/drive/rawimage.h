#ifndef OFSL_DRIVE_RAWIMAGE_H__
#define OFSL_DRIVE_RAWIMAGE_H__

#include <ofsl/drive/drive.h>

#ifdef __cplusplus
extern "C" {
#endif

OFSL_Drive* ofsl_drive_rawimage_create(const char* name, int readonly, size_t sector_size);

#ifdef __cplusplus
};
#endif

#endif
