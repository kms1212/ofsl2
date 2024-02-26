#ifndef OFSL_DRIVE_DRIVE_H__
#define OFSL_DRIVE_DRIVE_H__

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t lba_t;

typedef struct {
    uint16_t    sector_size;
    lba_t       lba_max;
    uint16_t    readonly : 1;
    uint16_t    : 15;
} OFSL_DriveInfo;

struct ofsl_drive_ops;

typedef struct ofsl_drive {
    const struct ofsl_drive_ops* ops;
    OFSL_DriveInfo drvinfo;
} OFSL_Drive;

struct ofsl_drive_ops {
    void (*_delete)(OFSL_Drive* drv);

    int (*update_info)(OFSL_Drive* drv);
    ssize_t (*read_sector)(OFSL_Drive* drv, void* buf, lba_t lba, size_t sector_size, size_t cnt);
    ssize_t (*write_sector)(OFSL_Drive* drv, const void* buf, lba_t lba, size_t sector_size, size_t cnt);
};

__attribute__((always_inline))
static inline void ofsl_drive_delete(OFSL_Drive* drv)
{
    drv->ops->_delete(drv);
}

__attribute__((always_inline))
static inline int ofsl_drive_update_info(OFSL_Drive* drv)
{
    return drv->ops->update_info(drv);
}

__attribute__((always_inline))
static inline ssize_t ofsl_drive_read_sector(OFSL_Drive* drv, void* buf, lba_t lba, size_t sector_size, size_t cnt)
{
    return drv->ops->read_sector(drv, buf, lba, sector_size, cnt);
}

__attribute__((always_inline))
static inline ssize_t ofsl_drive_write_sector(OFSL_Drive* drv, const void* buf, lba_t lba, size_t sector_size, size_t cnt)
{
    return drv->ops->write_sector(drv, buf, lba, sector_size, cnt);
}

#ifdef __cplusplus
};
#endif

#endif
