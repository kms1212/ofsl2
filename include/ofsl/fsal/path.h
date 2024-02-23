#ifndef OFSL_FSAL_PATH_H__
#define OFSL_FSAL_PATH_H__

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

char* fsal_path_concat(const char* path1, const char* path2, char* buf, size_t len);
char* fsal_path_resolve(const char* basepath, const char* relpath, char* buf, size_t len);

char* fsal_get_basename(const char* path, char* buf, size_t len);
char* fsal_get_extension(const char* path, char* buf, size_t len);
char* fsal_get_rootdir(const char* path, char* buf, size_t len);
char* fsal_get_fsid(const char* path, char* buf, size_t len);
char* fsal_get_parent(const char* path, char* buf, size_t len);

int fsal_path_is_valid(const char* path);
int fsal_path_is_relative(const char* path);
int fsal_path_has_filename(const char* path);
int fasl_path_has_extension(const char* path);

#ifdef __cplusplus
};
#endif

#endif
