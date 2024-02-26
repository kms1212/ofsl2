#ifndef OFSL_FSAL_FSAL_H__
#define OFSL_FSAL_FSAL_H__

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>

#include <ofsl/fs/fs.h>
#include <ofsl/drive/drive.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int a;
} FSAL_Directory;

typedef struct {
    int a;
} FSAL_File;

const char* fsal_register_drive(OFSL_Drive* drv, const char* drvtype);
void fsal_unregister_drive(const char* id);

int fsal_mount_fs(const char* id);
int fsal_unmount_fs(const char* id);

typedef struct {
    int a;
} FSAL_State;

FSAL_State* fsal_get_global_state(void);

int fsal_init_state(FSAL_State* state);

int fsal_gchdir(const char* pathname);
int fsal_chdir(FSAL_State* state, const char* pathname);

FSAL_Directory* fsal_gopendir(const char* pathname);
FSAL_Directory* fsal_opendir(FSAL_State* state, const char* pathname);

int fsal_gmkdir(const char* pathname);
int fsal_mkdir(FSAL_State* state, const char* pathname);

int fsal_gremove(const char* pathname);
int fsal_remove(FSAL_State* state, const char* pathname);

int fsal_grename(const char* oldpath, const char* newpath);
int fsal_rename(FSAL_State* state, const char* oldpath, const char* newpath);

const char* fsal_gpwd(void);
const char* fsal_pwd(FSAL_State* state);

FSAL_File* fsal_gfopen(const char* filename, const char* mode);
FSAL_File* fsal_fopen(FSAL_State* state, const char* filename, const char* mode);

const char* fsal_readdir(FSAL_Directory* dir);
void fsal_closedir(FSAL_Directory* dir);

#ifdef __cplusplus
};
#endif

#endif
