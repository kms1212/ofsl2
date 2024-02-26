#ifndef OFSL_PARTITION_PARTITION_H__
#define OFSL_PARTITION_PARTITION_H__

#include <ofsl/partition/errmsg.h>
#include <ofsl/drive/drive.h>

#ifdef __cplusplus
extern "C" {
#endif

struct ofsl_ptbl_ops;

typedef struct {
    const struct ofsl_ptbl_ops* ops;
    int error;
} OFSL_PartitionTable;

typedef struct {
    const struct ofsl_ptbl_ops* ops;
    OFSL_PartitionTable* pt;

    const char* part_name;
    OFSL_Drive* drv;
    lba_t lba_start;
    lba_t lba_end;
} OFSL_Partition;

struct ofsl_ptbl_ops {
    const char* (*get_error_string)(OFSL_PartitionTable* pt);
    void (*_delete)(OFSL_PartitionTable* pt);

    OFSL_Partition* (*list_start)(OFSL_PartitionTable* pt);
    int (*list_next)(OFSL_Partition* part);
    void (*list_end)(OFSL_Partition* part);
};

const char* ofsl_ptbl_get_error_string(OFSL_PartitionTable* pt);

__attribute__((always_inline))
static inline void ofsl_ptbl_delete(OFSL_PartitionTable* pt)
{
    pt->ops->_delete(pt);
}
__attribute__((always_inline))
static inline OFSL_Partition* ofsl_ptbl_list_start(OFSL_PartitionTable* pt)
{
    return pt->ops->list_start(pt);
}

__attribute__((always_inline))
static inline int ofsl_ptbl_list_next(OFSL_Partition* pinfo)
{
    return pinfo->pt->ops->list_next(pinfo);
}

__attribute__((always_inline))
static inline void ofsl_ptbl_list_end(OFSL_Partition* pinfo)
{
    pinfo->pt->ops->list_end(pinfo);
}

int ofsl_partition_from_drive(OFSL_Partition* part, OFSL_Drive* drv);

#ifdef __cplusplus
};
#endif

#endif
