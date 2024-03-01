#ifndef FS_ISO9660_INTERNAL_H__
#define FS_ISO9660_INTERNAL_H__

#include <stdint.h>

#include <ofsl/fs/iso9660.h>

#include "config.h"
#include "fs/iso9660/config.h"

#define ISO9660_SIGNATURE "CD001"

#define VDTYPE_BOOTRECORD   0
#define VDTYPE_PRIVOLDESC   1
#define VDTYPE_SUPVOLDESC   2
#define VDTYPE_VOLPARTDESC  3
#define VDTYPE_VDSETTERM    255

#if defined(BUILD_FILESYSTEM_ISO9660_ROCKRIDGE) || defined(BUILD_FILESYSTEM_ISO9660_JOILET)
#define ISO9660_MAX_PATH    255
#else
#define ISO9660_MAX_PATH    31
#endif
#define ISO9660_PATH_BUFSZ  (ISO9660_MAX_PATH + 1)

#ifdef BYTE_ORDER_BIG_ENDIAN
#define get_biendian_value(biend_struct) ((biend_struct)->be)

#else
#define get_biendian_value(biend_struct) ((biend_struct)->le)

#endif

struct biendian_pair_uint16 {
    uint16_t le;
    uint16_t be;
} OFSL_PACKED;

struct biendian_pair_int16 {
    int16_t le;
    int16_t be;
} OFSL_PACKED;

struct biendian_pair_uint32 {
    uint32_t le;
    uint32_t be;
} OFSL_PACKED;

struct biendian_pair_int32 {
    int32_t le;
    int32_t be;
} OFSL_PACKED;

struct isofs_time_longfmt {
    char year[4];
    char month[2];
    char day[2];
    char hour[2];
    char minute[2];
    char second[2];
    char ten_msec[2];
    uint8_t timezone;
} OFSL_PACKED;

struct isofs_time_shortfmt {
    uint8_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t timezone;
} OFSL_PACKED;

struct isofs_dir_entry_header {
    uint8_t entry_size;
    uint8_t attrib_record_size;
    struct biendian_pair_uint32 lba_data_location;
    struct biendian_pair_uint32 data_size;
    struct isofs_time_shortfmt created_time;
    uint8_t hidden : 1;
    uint8_t directory : 1;
    uint8_t associated : 1;
    uint8_t has_format_info : 1;
    uint8_t has_perm_info : 1;
    uint8_t : 2;
    uint8_t continued : 1;
    uint8_t interleave_file_unit_size;
    uint8_t interleave_gap_size;
    struct biendian_pair_uint16 vol_seq_num;
    uint8_t filename_len;
} OFSL_PACKED;

struct isofs_dir_entry_extension_header {
    uint8_t identifier[2];
    uint8_t entry_len;
    uint8_t entry_ver;
} OFSL_PACKED;

struct isofs_pathtbl_entry_header {
    uint8_t entry_len;
    uint8_t entry_len_extended;
    uint32_t lba_data;
    uint16_t parent_dir_idx;
    char name[];
} OFSL_PACKED;

struct isofs_vol_desc {
    uint8_t type;
    char    signature[5];
    uint8_t version;

    union {
        struct {
            char boot_sys_iden[32];
            char boot_iden[32];

            uint8_t sys_data[1977];
        } OFSL_PACKED boot_record;

        struct {
            uint8_t __reserved1;
            char sys_iden[32];
            char vol_name[32];
            uint8_t __reserved2[8];
            struct biendian_pair_uint32 vol_sector_count;
            uint8_t __reserved3[32];
            struct biendian_pair_uint16 vol_set_size;
            struct biendian_pair_uint16 vol_seq_num;
            struct biendian_pair_uint16 sector_size;
            struct biendian_pair_uint32 pathtbl_size;
            uint32_t lba_le_pathtbl;
            uint32_t lba_le_pathtbl_optional;
            uint32_t lba_be_pathtbl;
            uint32_t lba_be_pathtbl_optional;
            struct isofs_dir_entry_header rootdir_entry_header;
            uint8_t rootdir_entry_name;
            char vol_set_iden[128];
            char publisher_iden[128];
            char data_author_iden[128];
            char application_iden[128];
            char copyright_file_name[37];
            char abstract_file_name[37];
            char bibliographic_file_name[37];
            struct isofs_time_longfmt time_created;
            struct isofs_time_longfmt time_modified;
            struct isofs_time_longfmt time_expired_after;
            struct isofs_time_longfmt time_effective_after;
            uint8_t desc_ver;

            uint8_t __reserved4;
            uint8_t sys_data[512];
            uint8_t __reserved5[653];
        } OFSL_PACKED pvd;
    } OFSL_PACKED;
} OFSL_PACKED;

#ifdef BUILD_FILESYSTEM_ISO9660_ROCKRIDGE
enum posix_file_perm {
    PFP_EXEC = 1,
    PFP_WRITE = 2,
    PFP_READ = 4,
};

enum posix_file_type {
    PFT_FIFO = 1,
    PFT_CDEV = 2,
    PFT_DIR = 4,
    PFT_BDEV = 6,
    PFT_REG = 8,
    PFT_LNK = 10,
    PFT_SOCK = 12,
};

struct rrip_px_entry {
    struct isofs_dir_entry_extension_header header;
    struct biendian_pair_uint16 file_mode;
    struct biendian_pair_uint32 file_link;
    struct biendian_pair_uint32 uid;
    struct biendian_pair_uint32 gid;
    struct biendian_pair_uint32 file_serial;
} OFSL_PACKED;

struct rrip_nm_entry {
    struct isofs_dir_entry_extension_header header;
    uint8_t continue_name : 1;
    uint8_t current_directory : 1;
    uint8_t parent_directory : 1;
    uint8_t : 5;
    char filename[];
} OFSL_PACKED;

struct rrip_tf_entry {
    struct isofs_dir_entry_extension_header header;
    union {
        uint8_t flags_raw;

        struct {
            uint8_t creation_time : 1;
            uint8_t modification_time : 1;
            uint8_t access_time : 1;
            uint8_t attr_modification_time : 1;
            uint8_t backup_time : 1;
            uint8_t expiration_time : 1;
            uint8_t effective_time : 1;
            uint8_t long_format : 1;
        };
    };
} OFSL_PACKED;

#endif

#endif
