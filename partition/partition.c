#include <ofsl/partition/partition.h>

#include <ofsl/partition/errmsg.h>

static const char* error_str_list[] = {
    "Operation successfully finished",
    "No such partition entry",
};

const char* ofsl_ptbl_get_error_string(OFSL_PartitionTable* pt)
{
    if (pt->error >= 0) {
        if (pt->error > OFSL_PTE_MAX) {
            return error_str_list[pt->error];
        } else {
            return NULL;
        }
    } else {
        return pt->ops->get_error_string(pt);
    }
}

int ofsl_partition_from_drive(OFSL_Partition* part, OFSL_Drive* drive)
{
    part->drv = drive;
    part->lba_start = 0;
    part->lba_end = drive->drvinfo.lba_max;
    part->part_name = NULL;

    return 0;
}
