#ifndef OFSL_FS_FS_H__
#define OFSL_FS_FS_H__

#include <stdint.h>

#include <ofsl/fs/errmsg.h>
#include <ofsl/drive/drive.h>
#include <ofsl/time.h>

#ifndef SEEK_SET
#define SEEK_SET 0
#endif

#ifndef SEEK_CUR
#define SEEK_CUR 1
#endif

#ifndef SEEK_END
#define SEEK_END 2
#endif


#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char name[32];
    size_t max_file_name_length;
    size_t max_partition_sector_count;
    uint16_t unicode_supported : 1;
    uint16_t encryption_supported : 1;
    uint16_t compression_supported : 1;
    uint16_t permission_supported : 1;
} OFSL_FileSystemInfo;

typedef enum {
    OFSL_FTYPE_UNKNOWN = 0,
    OFSL_FTYPE_DIRECTORY,
    OFSL_FTYPE_FILE,
} OFSL_FileType;

typedef struct {
    OFSL_FileType type;
    uint16_t hidden : 1;
    uint16_t system : 1;
    uint16_t symlink : 1;
    uint16_t compressed : 1;
    uint16_t encrypted : 1;
    uint16_t immutable : 1;
    uint16_t : 10;
} OFSL_FileAttribute;

struct ofsl_fs_ops;

typedef struct {
    const struct ofsl_fs_ops* ops;
    int error;
} OFSL_FileSystem;

typedef struct {
    const struct ofsl_fs_ops* ops;
    OFSL_FileSystem* fs;
} OFSL_Directory;

typedef struct {
    const struct ofsl_fs_ops* ops;
    OFSL_FileSystem* fs;
} OFSL_File;

typedef struct {
    const struct ofsl_fs_ops* ops;
    OFSL_FileSystem* fs;
} OFSL_FileInfo;

struct ofsl_fs_ops {
    const char* (*get_error_string)(OFSL_FileSystem* fs);
    void (*_delete)(OFSL_FileSystem* fs);

    int (*mount)(OFSL_FileSystem* fs);
    int (*unmount)(OFSL_FileSystem* fs);

    const char* (*get_filesystem_name)(OFSL_FileSystem* fs);

    int (*dir_create)(OFSL_Directory* parent, const char* name);
    int (*dir_remove)(OFSL_Directory* parent, const char* name);
    OFSL_Directory* (*rootdir_open)(OFSL_FileSystem* fs);
    OFSL_Directory* (*dir_open)(OFSL_Directory* parent, const char* name);
    int (*dir_close)(OFSL_Directory* dir);
    OFSL_FileInfo* (*dir_list_start)(OFSL_Directory* dir);
    int (*dir_list_next)(OFSL_FileInfo* finfo);
    void (*dir_list_end)(OFSL_FileInfo* finfo);

    OFSL_FileInfo* (*get_file_info)(OFSL_Directory* parent, const char* name);
    const char* (*get_file_name)(const OFSL_FileInfo* finfo);
    OFSL_FileAttribute (*get_file_attrib)(const OFSL_FileInfo* finfo);
    ofsl_time_t (*get_file_created_time)(const OFSL_FileInfo* finfo);
    ofsl_time_t (*get_file_modified_time)(const OFSL_FileInfo* finfo);
    ofsl_time_t (*get_file_accessed_time)(const OFSL_FileInfo* finfo);
    ssize_t (*get_file_size)(const OFSL_FileInfo* finfo);

    int (*file_create)(OFSL_Directory* parent, const char* name);
    int (*file_remove)(OFSL_Directory* parent, const char* name);
    OFSL_File* (*file_open)(OFSL_Directory* parent, const char* name, const char* mode);
    int (*file_close)(OFSL_File* file);
    ssize_t (*file_read)(OFSL_File* file, void* buf, size_t size, size_t count);
    ssize_t (*file_write)(OFSL_File* file, const void* buf, size_t size, size_t count);
    int (*file_seek)(OFSL_File* file, ssize_t offset, int origin);
    int (*file_flush)(OFSL_File* file);
    ssize_t (*file_tell)(OFSL_File* file);
    int (*file_iseof)(OFSL_File* file);
};

const char* ofsl_fs_get_error_string(OFSL_FileSystem* fs);

__attribute__((always_inline))
static inline void ofsl_fs_delete(OFSL_FileSystem* fs)
{
    fs->ops->_delete(fs);
}

__attribute__((always_inline))
static inline int ofsl_fs_mount(OFSL_FileSystem* fs)
{
    return fs->ops->mount(fs);
}

__attribute__((always_inline))
static inline int ofsl_fs_unmount(OFSL_FileSystem* fs)
{
    return fs->ops->unmount(fs);
}

__attribute__((always_inline))
static inline const char* ofsl_fs_get_filesystem_name(OFSL_FileSystem* fs)
{
    return fs->ops->get_filesystem_name(fs);
}

__attribute__((always_inline))
static inline OFSL_Directory* ofsl_fs_rootdir_open(OFSL_FileSystem* fs)
{
    return fs->ops->rootdir_open(fs);
}

__attribute__((always_inline))
static inline int ofsl_dir_create(OFSL_Directory* parent, const char* name)
{
    return parent->ops->dir_create(parent, name);
}

__attribute__((always_inline))
static inline int ofsl_dir_remove(OFSL_Directory* parent, const char* name)
{
    return parent->ops->dir_remove(parent, name);
}

__attribute__((always_inline))
static inline OFSL_Directory* ofsl_dir_open(OFSL_Directory* parent, const char* name)
{
    return parent->ops->dir_open(parent, name);
}

__attribute__((always_inline))
static inline int ofsl_dir_close(OFSL_Directory* dir)
{
    return dir->ops->dir_close(dir);
}

__attribute__((always_inline))
static inline OFSL_FileInfo* ofsl_dir_list_start(OFSL_Directory* dir)
{
    return dir->ops->dir_list_start(dir);
}

__attribute__((always_inline))
static inline int ofsl_dir_list_next(OFSL_FileInfo* finfo)
{
    return finfo->ops->dir_list_next(finfo);
}

__attribute__((always_inline))
static inline void ofsl_dir_list_end(OFSL_FileInfo* finfo)
{
    finfo->ops->dir_list_end(finfo);
}

__attribute__((always_inline))
static inline OFSL_FileInfo* ofsl_get_file_info(OFSL_Directory* dir, const char* name)
{
    return dir->ops->get_file_info(dir, name);
}

__attribute__((always_inline))
static inline const char* ofsl_get_file_name(const OFSL_FileInfo* finfo)
{
    return finfo->ops->get_file_name(finfo);
}

__attribute__((always_inline))
static inline OFSL_FileAttribute ofsl_get_file_attrib(const OFSL_FileInfo* finfo)
{
    return finfo->ops->get_file_attrib(finfo);
}

__attribute__((always_inline))
static inline ofsl_time_t ofsl_get_file_created_time(const OFSL_FileInfo* finfo)
{
    return finfo->ops->get_file_created_time(finfo);
}

__attribute__((always_inline))
static inline ofsl_time_t ofsl_get_file_modified_time(const OFSL_FileInfo* finfo)
{
    return finfo->ops->get_file_modified_time(finfo);
}

__attribute__((always_inline))
static inline ofsl_time_t ofsl_get_file_accessed_time(const OFSL_FileInfo* finfo)
{
    return finfo->ops->get_file_accessed_time(finfo);
}

__attribute__((always_inline))
static inline ssize_t ofsl_get_file_size(const OFSL_FileInfo* finfo)
{
    return finfo->ops->get_file_size(finfo);
}

__attribute__((always_inline))
static inline int ofsl_file_create(OFSL_Directory* parent, const char* name)
{
    return parent->ops->file_create(parent, name);
}

__attribute__((always_inline))
static inline int ofsl_file_remove(OFSL_Directory* parent, const char* name)
{
    return parent->ops->file_remove(parent, name);
}

__attribute__((always_inline))
static inline OFSL_File* ofsl_file_open(OFSL_Directory* parent, const char* name, const char* mode)
{
    return parent->ops->file_open(parent, name, mode);
}

__attribute__((always_inline))
static inline int ofsl_file_close(OFSL_File* file)
{
    return file->ops->file_close(file);
}

__attribute__((always_inline))
static inline ssize_t ofsl_file_read(OFSL_File* file, void* buf, size_t size, size_t count)
{
    return file->ops->file_read(file, buf, size, count);
}

__attribute__((always_inline))
static inline ssize_t ofsl_file_write(OFSL_File* file, const void* buf, size_t size, size_t count)
{
    return file->ops->file_write(file, buf, size, count);
}

__attribute__((always_inline))
static inline int ofsl_file_seek(OFSL_File* file, ssize_t offset, int origin)
{
    return file->ops->file_seek(file, offset, origin);
}

__attribute__((always_inline))
static inline int ofsl_file_flush(OFSL_File* file)
{
    return file->ops->file_flush(file);
}

__attribute__((always_inline))
static inline ssize_t ofsl_file_tell(OFSL_File* file)
{
    return file->ops->file_tell(file);
}

__attribute__((always_inline))
static inline int ofsl_file_iseof(OFSL_File* file)
{
    return file->ops->file_iseof(file);
}

#ifdef __cplusplus
};
#endif

#endif
