#include <ofsl/ptbl/ptbl.h>

#include <ofsl/ptbl/errmsg.h>

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
