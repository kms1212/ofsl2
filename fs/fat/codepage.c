#include "fs/fat/codepage.h"

#include <stddef.h>

#include "export.h"

OFSL_HIDDEN
const char* get_uppercase_table(int codepage)
{
    switch (codepage) {
        case 437:
            return cp437_uctable;
        case 720:
            return cp720_uctable;
        case 737:
            return cp737_uctable;
        case 771:
            return cp771_uctable;
        case 775:
            return cp775_uctable;
        case 850:
            return cp850_uctable;
        case 852:
            return cp852_uctable;
        case 855:
            return cp855_uctable;
        case 857:
            return cp857_uctable;
        case 860:
            return cp860_uctable;
        case 861:
            return cp861_uctable;
        case 862:
            return cp862_uctable;
        case 863:
            return cp863_uctable;
        case 864:
            return cp864_uctable;
        case 865:
            return cp865_uctable;
        case 866:
            return cp866_uctable;
        case 869:
            return cp869_uctable;
        default:
            return NULL;
    }
}
