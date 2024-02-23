#ifndef OFSL_PTBL_PTBL_H__
#define OFSL_PTBL_PTBL_H__

#include <ofsl/ptbl/errmsg.h>
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
} OFSL_PartitionInfo;

struct ofsl_ptbl_ops {
    const char* (*get_error_string)(OFSL_PartitionTable* pt);
    void (*_delete)(OFSL_PartitionTable* pt);

    OFSL_PartitionInfo* (*list_start)(OFSL_PartitionTable* pt);
    int (*list_next)(OFSL_PartitionInfo* pinfo);
    void (*list_end)(OFSL_PartitionInfo* pinfo);

    lba_t (*get_part_lba_start)(OFSL_PartitionInfo* pinfo);
    lba_t (*get_part_lba_end)(OFSL_PartitionInfo* pinfo);
    const char* (*get_part_name)(OFSL_PartitionInfo* pinfo);
};

const char* ofsl_ptbl_get_error_string(OFSL_PartitionTable* pt);

__attribute__((always_inline))
static inline void ofsl_ptbl_delete(OFSL_PartitionTable* pt)
{
    pt->ops->_delete(pt);
}
__attribute__((always_inline))
static inline OFSL_PartitionInfo* ofsl_ptbl_list_start(OFSL_PartitionTable* pt)
{
    return pt->ops->list_start(pt);
}

__attribute__((always_inline))
static inline int ofsl_ptbl_list_next(OFSL_PartitionInfo* pinfo)
{
    return pinfo->pt->ops->list_next(pinfo);
}

__attribute__((always_inline))
static inline void ofsl_ptbl_list_end(OFSL_PartitionInfo* pinfo)
{
    pinfo->pt->ops->list_end(pinfo);
}

__attribute__((always_inline))
static inline lba_t ofsl_get_part_lba_start(OFSL_PartitionInfo* pinfo)
{
    return pinfo->pt->ops->get_part_lba_start(pinfo);
}

__attribute__((always_inline))
static inline lba_t ofsl_get_part_lba_end(OFSL_PartitionInfo* pinfo)
{
    return pinfo->pt->ops->get_part_lba_end(pinfo);
}

__attribute__((always_inline))
static inline const char* ofsl_get_part_name(OFSL_PartitionInfo* pinfo)
{
    return pinfo->pt->ops->get_part_name(pinfo);
}

#ifdef __cplusplus
};
#endif

#endif
