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
    size_t size;
    uint16_t directory : 1;
    uint16_t hidden : 1;
    uint16_t system : 1;
    uint16_t link : 1;
    uint16_t symlink : 1;
    uint16_t compressed : 1;
    uint16_t encrypted : 1;
    uint16_t immutable : 1;
    uint16_t : 9;
    OFSL_Time time_created;
    OFSL_Time time_modified;
    OFSL_Time time_accessed;
} OFSL_FileAttribute;

typedef struct {
    const char* fs_name;
    const char* vol_name;
    const char* vol_serial;
} OFSL_MountInfo;

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
} OFSL_DirectoryIterator;

typedef struct {
    const struct ofsl_fs_ops* ops;
    OFSL_FileSystem* fs;
} OFSL_File;

struct ofsl_fs_ops {
    const char* (*get_error_string)(OFSL_FileSystem* fs);
    void (*_delete)(OFSL_FileSystem* fs);

    int (*mount)(OFSL_FileSystem* fs);
    int (*unmount)(OFSL_FileSystem* fs);

    int (*get_mount_info)(OFSL_FileSystem* fs, OFSL_MountInfo* mntinfo);

    int (*dir_create)(OFSL_Directory* parent, const char* name);
    int (*dir_remove)(OFSL_Directory* parent, const char* name);
    OFSL_Directory* (*rootdir_open)(OFSL_FileSystem* fs);
    OFSL_Directory* (*dir_open)(OFSL_Directory* parent, const char* name);
    int (*dir_close)(OFSL_Directory* dir);

    OFSL_DirectoryIterator* (*dir_iter_start)(OFSL_Directory* dir);
    int (*dir_iter_next)(OFSL_DirectoryIterator* it);
    void (*dir_iter_end)(OFSL_DirectoryIterator* it);
    const char* (*dir_iter_get_file_name)(const OFSL_DirectoryIterator* it);
    int (*dir_iter_get_attr)(const OFSL_DirectoryIterator* it, OFSL_FileAttribute* fattr);

    int (*get_file_attr)(OFSL_Directory* parent, const char* name, OFSL_FileAttribute* fattr);

    int (*file_create)(OFSL_Directory* parent, const char* name);
    int (*file_remove)(OFSL_Directory* parent, const char* name);
    OFSL_File* (*file_open)(OFSL_Directory* parent, const char* name, const char* mode);
    int (*file_close)(OFSL_File* file);
    ssize_t (*file_read)(OFSL_File* file, void* buf, size_t size, size_t count);
    ssize_t (*file_write)(OFSL_File* file, const void* buf, size_t size, size_t count);
    int (*file_seek)(OFSL_File* file, ssize_t offset, int origin);
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
static inline int ofsl_fs_get_mount_info(OFSL_FileSystem* fs, OFSL_MountInfo* mntinfo)
{
    return fs->ops->get_mount_info(fs, mntinfo);
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
static inline OFSL_DirectoryIterator* ofsl_dir_iter_start(OFSL_Directory* dir)
{
    return dir->ops->dir_iter_start(dir);
}

__attribute__((always_inline))
static inline int ofsl_dir_iter_next(OFSL_DirectoryIterator* it)
{
    return it->ops->dir_iter_next(it);
}

__attribute__((always_inline))
static inline void ofsl_dir_iter_end(OFSL_DirectoryIterator* it)
{
    it->ops->dir_iter_end(it);
}

__attribute__((always_inline))
static inline const char* ofsl_dir_iter_get_file_name(const OFSL_DirectoryIterator* it)
{
    return it->ops->dir_iter_get_file_name(it);
}

__attribute__((always_inline))
static inline int ofsl_dir_iter_get_attr(const OFSL_DirectoryIterator* it, OFSL_FileAttribute* fattr)
{
    return it->ops->dir_iter_get_attr(it, fattr);
}

__attribute__((always_inline))
static inline int ofsl_get_file_attr(OFSL_Directory* parent, const char* name, OFSL_FileAttribute* fattr)
{
    return parent->ops->get_file_attr(parent, name, fattr);
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
