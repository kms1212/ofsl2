#ifndef OFSL_FS_FAT_H__
#define OFSL_FS_FAT_H__

#include <ofsl/fs/fs.h>

#ifdef __cplusplus
extern "C" {
#endif

struct ofsl_fs_fat_option {
    unsigned int diskbuf_count;
    uint8_t     lfn_enabled : 1;
    uint8_t     unicode_enabled : 1;
    uint8_t     use_fsinfo_nextfree : 1;
    uint8_t     case_sensitive : 1;
    uint8_t     sfn_lowercase : 1;
    uint8_t     readonly : 1;
    char        unknown_char_fallback;
};

OFSL_FileSystem* ofsl_create_fs_fat(OFSL_Drive* drv, lba_t lba_offs);

struct ofsl_fs_fat_option* ofsl_fs_fat_get_option(OFSL_FileSystem* fs);

#ifdef __cplusplus
};
#endif

#endif
