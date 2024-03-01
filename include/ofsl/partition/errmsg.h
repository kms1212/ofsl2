#ifndef OFSL_PARTITION_ERRMSG_H__
#define OFSL_PARTITION_ERRMSG_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    OFSL_PTE_SUCCESS    = 0,
    OFSL_PTE_NOENT      = 1,
    OFSL_PTE_MAX        = 2,  /* should be equal to last enum value */
} OFSL_PartitionTableError;

#ifdef __cplusplus
};
#endif

#endif
