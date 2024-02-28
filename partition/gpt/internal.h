#ifndef PTBL_GPT_INTERNAL_H__
#define PTBL_GPT_INTERNAL_H__

#include <stdint.h>

#include "config.h"

#define MBR_PTYPE_GPT 0xEE

struct mbr_part_entry {
    uint8_t boot_flag;
    uint8_t chs_start[3];
    uint8_t type;
    uint8_t chs_end[3];
    uint32_t lba_start;
    uint32_t lba_size;
} OFSL_PACKED;

struct mbr_table_sector {
    uint8_t boot_code[446];
    struct mbr_part_entry partition_entry[4];
    uint16_t signature;
} OFSL_PACKED;

struct gpt_table_header {
    char signature[8];
    uint32_t revision;
    uint32_t header_size;
    uint32_t header_crc32;
    uint8_t __reserved1[4];
    uint64_t lba_header_current;
    uint64_t lba_header_backup;
    uint64_t lba_partarea_start;
    uint64_t lba_partarea_end;
    uint8_t disk_guid[16];
    uint64_t lba_part_entry_start;
    uint32_t part_entry_count;
    uint32_t part_entry_size;
    uint32_t part_entry_crc32;
    uint8_t __reserved2[420];
} OFSL_PACKED;

struct gpt_part_entry {
    uint8_t type_guid[16];
    uint8_t guid[16];
    uint64_t lba_start;
    uint64_t lba_end;
    uint64_t flags;
    uint16_t name[36];
} OFSL_PACKED;

#endif
