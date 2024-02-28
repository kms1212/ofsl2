#ifndef OFSL_FS_ISO9660_H__
#define OFSL_FS_ISO9660_H__

#include <ofsl/fs/fs.h>
#include <ofsl/partition/partition.h>
#include <ofsl/time.h>

#ifdef __cplusplus
extern "C" {
#endif

struct ofsl_fs_iso9660_option {
    unsigned int diskbuf_count;
    uint8_t case_sensitive : 1;
    uint8_t enable_rock_ridge : 1;
    uint8_t enable_joilet : 1;
};

struct ofsl_fs_iso9660_vol_info {
    char system_name[33];
    char volume_name[33];
    char publisher[129];
    char author[129];
    char application[129];
    char copyright_file_path[38];
    char abstract_file_path[38];
    char bibliography_file_path[38];
    OFSL_Time time_created;
    OFSL_Time time_modified;
    OFSL_Time time_expired;
    OFSL_Time time_effective;
};

OFSL_FileSystem* ofsl_fs_iso9660_create(OFSL_Partition* part);

struct ofsl_fs_iso9660_option* ofsl_fs_iso9660_get_option(OFSL_FileSystem* fs);


#ifdef __cplusplus
};
#endif

#endif
