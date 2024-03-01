#ifndef OFSL_FS_FAT_H__
#define OFSL_FS_FAT_H__

#include <ofsl/fs/fs.h>
#include <ofsl/partition/partition.h>

#ifdef __cplusplus
extern "C" {
#endif

struct ofsl_fs_fat_option {
    unsigned int diskbuf_count;
    unsigned int codepage;
    uint8_t     lfn_enabled : 1;
    uint8_t     unicode_enabled : 1;
    uint8_t     use_fsinfo_nextfree : 1;
    uint8_t     case_sensitive : 1;
    uint8_t     sfn_lowercase : 1;
    uint8_t     readonly : 1;
    char        unknown_char_fallback;
};

OFSL_FileSystem* ofsl_fs_fat_create(OFSL_Partition* part);

struct ofsl_fs_fat_option* ofsl_fs_fat_get_option(OFSL_FileSystem* fs);

#ifdef __cplusplus
};
#endif

#endif
