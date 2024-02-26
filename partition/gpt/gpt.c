#include <ofsl/partition/gpt.h>

#include <stdlib.h>
#include <string.h>

#include "partition/gpt/internal.h"
#include "crypto/crc32.h"

struct ptbl_gpt {
    OFSL_PartitionTable ptbl;
    OFSL_Drive* drv;
    lba_t lba_gpt_header;
    lba_t lba_partarea_start;
    lba_t lba_partarea_end;
    lba_t lba_part_entry_start;
    uint32_t part_entry_count;
};

struct part_gpt {
    OFSL_Partition part;
    uint32_t idx;
    int valid;
    char name[36];
};

static const char* error_str_list[] = {
    "Error message not available",
};

static struct ptbl_gpt* check_ptbl(OFSL_PartitionTable* pt_opaque)
{
    return (struct ptbl_gpt*)pt_opaque;
}

static struct part_gpt* check_part(OFSL_Partition* pinfo_opaque)
{
    return (struct part_gpt*)pinfo_opaque;
}

static const char* get_error_string(OFSL_PartitionTable* pt_opaque)
{
    struct ptbl_gpt* pt = check_ptbl(pt_opaque);
    if (!pt_opaque) {
        return NULL;
    }

    if (pt->ptbl.error < 0) {
        return error_str_list[-pt->ptbl.error - 1];
    } else {
        return NULL;
    }
}

static void _delete(OFSL_PartitionTable* pt)
{
    free(pt);
}

static OFSL_Partition* list_start(OFSL_PartitionTable* pt_opaque)
{
    struct ptbl_gpt* pt = check_ptbl(pt_opaque);
    if (!pt) {
        return NULL;
    }

    struct part_gpt* part = malloc(sizeof(struct part_gpt));
    part->part.ops = pt->ptbl.ops;
    part->part.pt = (OFSL_PartitionTable*)pt;
    part->valid = 0;
    return (OFSL_Partition*)part;
}

static int list_next(OFSL_Partition* part_opaque)
{
    struct part_gpt* part = check_part(part_opaque);
    if (!part) {
        return 1;
    } else if (!part->valid) {
        part->idx = 0;
    }

    struct ptbl_gpt* pt = check_ptbl(part->part.pt);
    if (!pt) {
        return 1;
    } else if (part->idx >= pt->part_entry_count) {
        return 1;
    }

    uint8_t sector_buf[512];
    ofsl_drive_read_sector(pt->drv, sector_buf, pt->lba_part_entry_start + (part->idx / 4), sizeof(sector_buf), 1);
    struct gpt_part_entry* pent = &((struct gpt_part_entry*)sector_buf)[part->idx % 4];

    uint16_t guid_sum = 0;
    for (int i = 0; i < 16; i++) {
        guid_sum += pent->type_guid[i];
    }
    if (!guid_sum) {
        return 1;
    }

    part->part.lba_start = pent->lba_start;
    part->part.lba_end = pent->lba_end;
    part->part.part_name = part->name;
    part->valid = 1;
    for (int i = 0; i < 36; i++) {
        part->name[i] = pent->name[i];
    }

    part->idx++;

    return 0;
}

static void list_end(OFSL_Partition* pinfo)
{
    free(pinfo);
}

OFSL_PartitionTable* ofsl_ptbl_gpt_create(OFSL_Drive* drv)
{
    static const struct ofsl_ptbl_ops ptops = {
        .get_error_string = get_error_string,
        ._delete = _delete,
        .list_start = list_start,
        .list_next = list_next,
        .list_end = list_end,
    };
    
    /* verify MBR */
    uint8_t sector_buf[512];
    ofsl_drive_read_sector(drv, sector_buf, 0, sizeof(sector_buf), 1);
    struct mbr_table_sector* protmbr = (struct mbr_table_sector*)sector_buf;
    if (protmbr->signature != 0xAA55 || protmbr->partition_entry[0].type != MBR_PTYPE_GPT) {
        return NULL;
    }
    lba_t lba_gpt_header = protmbr->partition_entry[0].lba_start;

    /* verify GPT header */
    ofsl_drive_read_sector(drv, sector_buf, lba_gpt_header, sizeof(sector_buf), 1);
    struct gpt_table_header* gpthdr = (struct gpt_table_header*)sector_buf;
    if (strncmp(gpthdr->signature, "EFI PART", 8) != 0) {
        return NULL;
    }
    uint32_t original_crc32 = gpthdr->header_crc32;
    gpthdr->header_crc32 = 0;
    if (original_crc32 != ofsl_gen_crc32(0, gpthdr, 0x5c)) {
        return NULL;
    }

    /* allocate object and set value */
    struct ptbl_gpt* pt = malloc(sizeof(struct ptbl_gpt));
    pt->ptbl.ops = &ptops;
    pt->drv = drv;
    pt->lba_gpt_header = lba_gpt_header;
    pt->lba_part_entry_start = gpthdr->lba_part_entry_start;
    pt->lba_partarea_start = gpthdr->lba_partarea_start;
    pt->lba_partarea_end = gpthdr->lba_partarea_end;
    pt->part_entry_count = gpthdr->part_entry_count;

    return (OFSL_PartitionTable*)pt;
}
