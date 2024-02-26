#ifndef FS_ISO9660_INTERNAL_H__
#define FS_ISO9660_INTERNAL_H__

#include <stdint.h>

#include <ofsl/fs/iso9660.h>

#define ISO9660_SIGNATURE "CD001"

#define VDTYPE_BOOTRECORD   0
#define VDTYPE_PRIVOLDESC   1
#define VDTYPE_SUPVOLDESC   2
#define VDTYPE_VOLPARTDESC  3
#define VDTYPE_VDSETTERM    255

struct biendian_pair_uint16 {
    uint16_t be;
    uint16_t le;
} __attribute__((packed));

struct biendian_pair_int16 {
    int16_t be;
    int16_t le;
} __attribute__((packed));

struct biendian_pair_uint32 {
    uint32_t be;
    uint32_t le;
} __attribute__((packed));

struct biendian_pair_int32 {
    int32_t be;
    int32_t le;
} __attribute__((packed));

struct isofs_vol_datetime {
    char year[4];
    char month[2];
    char day[2];
    char hour[2];
    char minute[2];
    char second[2];
    char ten_msec[2];
    uint8_t timezone;
} __attribute__((packed));

struct isofs_entry_datetime {
    uint8_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t timezone;
} __attribute__((packed));

struct isofs_dir_entry_header {
    uint8_t entry_size;
    uint8_t attrib_record_size;
    struct biendian_pair_uint32 lba_data_location;
    struct biendian_pair_uint32 lba_data_size;
    struct isofs_entry_datetime created_time;
    uint8_t flags;
    uint8_t interleave_file_unit_size;
    uint8_t interleave_gap_size;
    struct biendian_pair_uint16 vol_seq_num;
    uint8_t filename_len;
} __attribute__((packed));

struct isofs_vol_desc {
    uint8_t type;
    char    signature[5];
    uint8_t version;

    union {
        struct {
            char boot_sys_iden[32];
            char boot_iden[32];

            uint8_t sys_data[1977];
        } __attribute__((packed)) boot_record;

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
            struct biendian_pair_uint32 path_table_size;
            uint32_t lba_le_path_table_typel;
            uint32_t lba_le_path_table_typel_optional;
            uint32_t lba_be_path_table_typel;
            uint32_t lba_be_path_table_typel_optional;
            struct isofs_dir_entry_header rootdir_entry_header;
            uint8_t rootdir_entry_name;
            char vol_set_iden[128];
            char publisher_iden[128];
            char data_author_iden[128];
            char application_iden[128];
            char copyright_file_name[37];
            char abstract_file_name[37];
            char bibliographic_file_name[37];
            struct isofs_vol_datetime time_created;
            struct isofs_vol_datetime time_modified;
            struct isofs_vol_datetime time_expired_after;
            struct isofs_vol_datetime time_effective_after;
            uint8_t descriptor_version;

            uint8_t __reserved4;
            uint8_t sys_data[512];
            uint8_t __reserved5[653];
        } __attribute__((packed)) primary_vol_desc;
    } __attribute__((packed));
} __attribute__((packed));

#endif
