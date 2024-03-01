#ifndef OFSL_FS_ERRMSG_H__
#define OFSL_FS_ERRMSG_H__


#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    OFSL_FSE_SUCCESS    = 0,
    OFSL_FSE_NOENT      = 1,
    OFSL_FSE_UNMOUNTED  = 2,
    OFSL_FSE_ICLUSTER   = 3,
    OFSL_FSE_INVALFS    = 4,
    OFSL_FSE_IENTNAME   = 5,
    OFSL_FSE_MAX        = 5,  /* should be equal to last enum value */
} OFSL_FileSystemError;

#ifdef __cplusplus
};
#endif

#endif
