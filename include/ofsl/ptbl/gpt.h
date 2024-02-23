#ifndef OFSL_PTBL_GPT_H__
#define OFSL_PTBL_GPT_H__

#include <ofsl/ptbl/ptbl.h>

#ifdef __cplusplus
extern "C" {
#endif

OFSL_PartitionTable* ofsl_create_ptbl_gpt(OFSL_Drive* drv);

#ifdef __cplusplus
};
#endif

#endif
