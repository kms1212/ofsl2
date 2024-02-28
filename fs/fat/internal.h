#ifndef FS_FAT_INTERNAL_H__
#define FS_FAT_INTERNAL_H__

#include <stdint.h>

#include <ofsl/fs/fat.h>

#include "config.h"
#include "fs/fat/config.h"

#define FAT_TYPE_UNKNOWN    0
#define FAT_TYPE_FAT12      1
#define FAT_TYPE_FAT16      2
#define FAT_TYPE_FAT32      3

#define FAT_SFN_NAME            8
#define FAT_SFN_EXTENSION       3
#define FAT_SFN_LENGTH          (FAT_SFN_NAME + FAT_SFN_EXTENSION)
#define FAT_SFN_BUFLEN          FAT_SFN_LENGTH + 2  // "filename" + '.' + "ext" + '\0'

#ifdef BUILD_FILESYSTEM_FAT_LFN
#define FAT_LFN_LENGTH          255
#define FAT_LFN_BUFLEN          FAT_LFN_LENGTH + 1  // "longfilename" + '\0'
#define FAT_LFN_U8_BUFLEN       384
#define FAT_FILENAME_BUF_LEN    FAT_LFN_U8_BUFLEN

#else
#define FAT_FILENAME_BUF_LEN    FAT_SFN_BUFLEN

#endif

#define FAT_SECTOR_SIZE         512

#define FAT_LFN_END_MASK        0x40

#define FAT12_MAX_CLUSTER       0xFF4
#define FAT16_MAX_CLUSTER       0xFFF4
#define FAT32_MAX_CLUSTER       0x0FFFFFF6

#define FAT12_BAD_CLUSTER       0xFF7
#define FAT16_BAD_CLUSTER       0xFFF7
#define FAT32_BAD_CLUSTER       0x0FFFFFF7

#define FAT12_END_CLUSTER       0xFFF
#define FAT16_END_CLUSTER       0xFFFF
#define FAT32_END_CLUSTER       0x0FFFFFFF

#define FAT_BPB_SIGNATURE       0xAA55

#define FAT_FSINFO_SIGNATURE1   0x41615252
#define FAT_FSINFO_SIGNATURE2   0x61417272
#define FAT_FSINFO_SIGNATURE3   (FAT_BPB_SIGNATURE)

#define FAT_ATTR_READ_ONLY      0x01
#define FAT_ATTR_HIDDEN         0x02
#define FAT_ATTR_SYSTEM         0x04
#define FAT_ATTR_VOLUME_ID      0x08
#define FAT_ATTR_DIRECTORY      0x10
#define FAT_ATTR_ARCHIVE        0x20

#define FAT_ATTR_LFNENTRY       (FAT_ATTR_READ_ONLY | FAT_ATTR_HIDDEN | FAT_ATTR_SYSTEM | FAT_ATTR_VOLUME_ID)

union fat_time {
    uint16_t raw;
    struct {
        uint16_t second_div2 : 5;
        uint16_t minute : 6;
        uint16_t hour : 5;
    };
} OFSL_PACKED;

union fat_date {
    uint16_t raw;
    struct {
        uint16_t day : 5;
        uint16_t month : 4;
        uint16_t year : 7;
    };
} OFSL_PACKED;

struct fat_bpb_sector {
    uint8_t         x86_jump_code[3];
    char            oem_name[8];
    uint16_t        bytes_per_sector;
    uint8_t         sectors_per_cluster;
    uint16_t        reserved_sector_count;
    uint8_t         fat_count;
    uint16_t        root_entry_count;
    uint16_t        total_sector_count16;
    uint8_t         media_type;
    uint16_t        fat_size16;
    uint16_t        sectors_per_track;
    uint16_t        head_count;
    uint32_t        hidden_sector_count;
    uint32_t        total_sector_count32;

    union {
        struct {
            uint8_t         drive_num;
            uint8_t         __reserved1;
            uint8_t         boot_signature;
            uint32_t        volume_serial;
            char            volume_label[11];
            char            fs_type[8];

            uint8_t         boot_code[448];
        } OFSL_PACKED fat;

        struct {
            uint32_t        fat_size32;
            uint16_t        flags;
            uint16_t        version;
            uint32_t        root_cluster;
            uint16_t        fsinfo_sector;
            uint16_t        bpb_backup_sector;
            uint8_t         __reserved1[12];
            uint8_t         physical_drive_num;
            uint8_t         __reserved2;
            uint8_t         extended_boot_signature;
            uint32_t        volume_serial;
            char            volume_label[11];
            char            fs_type[8];

            uint8_t         boot_code[420];
        } OFSL_PACKED fat32;
    };

    uint16_t        signature;
} OFSL_PACKED;

struct fat_fsinfo {
    uint32_t        signature1;
    uint8_t         __reserved1[480];
    uint32_t        signature2;
    uint32_t        free_clusters;
    uint32_t        next_free_cluster;
    uint8_t         __reserved2[14];
    uint16_t        signature3;
} OFSL_PACKED;

union fat_dir_entry {
    struct fat_direntry_file {
        uint8_t         name[FAT_SFN_NAME];
        uint8_t         extension[FAT_SFN_EXTENSION];
        uint8_t         attribute;
        uint8_t         __reserved;
        uint8_t         created_tenth;
        union fat_time  created_time;
        union fat_date  created_date;
        union fat_date  accessed_date;
        uint16_t        cluster_location_high;
        union fat_time  modified_time;
        union fat_date  modified_date;
        uint16_t        cluster_location;
        uint32_t        size;
    } OFSL_PACKED file;

    struct fat_direntry_lfn {
        uint8_t         sequence_index;
        uint16_t        name_fragment1[5];
        uint8_t         attribute;
        uint8_t         __reserved;
        uint8_t         checksum;
        uint16_t        name_fragment2[6];
        uint16_t        cluster_location;
        uint16_t        name_fragment3[2];
    } OFSL_PACKED lfn;
} OFSL_PACKED;

typedef uint32_t fatcluster_t;

#endif
