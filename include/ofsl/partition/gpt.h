#ifndef OFSL_PARTITION_GPT_H__
#define OFSL_PARTITION_GPT_H__

#include <ofsl/partition/partition.h>

#ifdef __cplusplus
extern "C" {
#endif

OFSL_PartitionTable* ofsl_ptbl_gpt_create(OFSL_Drive* drv);

#ifdef __cplusplus
};
#endif

#endif
