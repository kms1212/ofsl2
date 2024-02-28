#ifndef OFSL_FS_FS_H__
#define OFSL_FS_FS_H__

#include <stdint.h>

#include <ofsl/fs/errmsg.h>
#include <ofsl/drive/drive.h>
#include <ofsl/time.h>
#include <ofsl/conf.h>

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

typedef enum {
    OFSL_FTYPE_ERROR = 0,
    OFSL_FTYPE_DIR,
    OFSL_FTYPE_FILE,
    OFSL_FTYPE_LINK,
    OFSL_FTYPE_OTHER,
} OFSL_FileType;

typedef enum {
    OFSL_VSTYPE_LABEL,
    OFSL_VSTYPE_SERIAL,
    OFSL_VSTYPE_PROGRAM,
    OFSL_VSTYPE_PUBLISHER,
    OFSL_VSTYPE_AUTHOR,
    OFSL_VSTYPE_COPYRIGHT,
    OFSL_VSTYPE_ABSTRACT,
    OFSL_VSTYPE_BIBLIOGRAPHY,
} OFSL_VolumeStringType;

typedef enum {
    OFSL_TSTYPE_CREATION,
    OFSL_TSTYPE_MODIFICATION,
    OFSL_TSTYPE_ATTR_MODIFICATION,
    OFSL_TSTYPE_ACCESS,
    OFSL_TSTYPE_DELETION,
    OFSL_TSTYPE_EFFECTIVE,
    OFSL_TSTYPE_EXPIRATION,
    OFSL_TSTYPE_BACKUP,
} OFSL_TimestampType;

typedef enum {
    OFSL_FATYPE_HIDDEN,
    OFSL_FATYPE_SYSTEM,
    OFSL_FATYPE_NODUMP,
    OFSL_FATYPE_READONLY,
    OFSL_FATYPE_APPENDONLY,
    OFSL_FATYPE_OPAQUE,
    OFSL_FATYPE_NOUNLINK,
    OFSL_FATYPE_NOARCHIVE,
    OFSL_FATYPE_SNAPSHOT,
    OFSL_FATYPE_NOHISTORY,
    OFSL_FATYPE_ARCHIVED,
    OFSL_FATYPE_COMPRESSED,
} OFSL_FileAttributeType;

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

    const char* (*get_fs_name)(OFSL_FileSystem* fs);
    int (*get_volume_string)(OFSL_FileSystem* fs, OFSL_VolumeStringType type, char* buf, size_t len);
    int (*get_volume_timestamp)(OFSL_FileSystem* fs, OFSL_TimestampType type, OFSL_Time* time);

    int (*dir_create)(OFSL_Directory* parent, const char* name);
    int (*dir_remove)(OFSL_Directory* parent, const char* name);
    OFSL_Directory* (*rootdir_open)(OFSL_FileSystem* fs);
    OFSL_Directory* (*dir_open)(OFSL_Directory* parent, const char* name);
    int (*dir_close)(OFSL_Directory* dir);

    OFSL_DirectoryIterator* (*dir_iter_start)(OFSL_Directory* dir);
    int (*dir_iter_next)(OFSL_DirectoryIterator* it);
    void (*dir_iter_end)(OFSL_DirectoryIterator* it);

    const char* (*dir_iter_get_name)(OFSL_DirectoryIterator* it);
    OFSL_FileType (*dir_iter_get_type)(OFSL_DirectoryIterator* it);
    int (*dir_iter_get_timestamp)(OFSL_DirectoryIterator* it, OFSL_TimestampType type, OFSL_Time* time);
    int (*dir_iter_get_attr)(OFSL_DirectoryIterator* it, OFSL_FileAttributeType type);
    int (*dir_iter_get_size)(OFSL_DirectoryIterator* it, size_t* size);

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

OFSL_INLINE
static inline void ofsl_fs_delete(OFSL_FileSystem* fs)
{
    fs->ops->_delete(fs);
}

OFSL_INLINE
static inline int ofsl_fs_mount(OFSL_FileSystem* fs)
{
    return fs->ops->mount(fs);
}

OFSL_INLINE
static inline int ofsl_fs_unmount(OFSL_FileSystem* fs)
{
    return fs->ops->unmount(fs);
}

OFSL_INLINE
static inline const char* ofsl_fs_get_fs_name(OFSL_FileSystem* fs)
{
    return fs->ops->get_fs_name(fs);
}

OFSL_INLINE
static inline int ofsl_fs_get_volume_string(OFSL_FileSystem* fs, OFSL_VolumeStringType type, char* buf, size_t len)
{
    return fs->ops->get_volume_string(fs, type, buf, len);
}

OFSL_INLINE
static inline int ofsl_fs_get_volume_timestamp(OFSL_FileSystem* fs, OFSL_TimestampType type, OFSL_Time* time)
{
    return fs->ops->get_volume_timestamp(fs, type, time);
}

OFSL_INLINE
static inline OFSL_Directory* ofsl_fs_rootdir_open(OFSL_FileSystem* fs)
{
    return fs->ops->rootdir_open(fs);
}

OFSL_INLINE
static inline int ofsl_dir_create(OFSL_Directory* parent, const char* name)
{
    return parent->ops->dir_create(parent, name);
}

OFSL_INLINE
static inline int ofsl_dir_remove(OFSL_Directory* parent, const char* name)
{
    return parent->ops->dir_remove(parent, name);
}

OFSL_INLINE
static inline OFSL_Directory* ofsl_dir_open(OFSL_Directory* parent, const char* name)
{
    return parent->ops->dir_open(parent, name);
}

OFSL_INLINE
static inline int ofsl_dir_close(OFSL_Directory* dir)
{
    return dir->ops->dir_close(dir);
}

OFSL_INLINE
static inline OFSL_DirectoryIterator* ofsl_dir_iter_start(OFSL_Directory* dir)
{
    return dir->ops->dir_iter_start(dir);
}

OFSL_INLINE
static inline int ofsl_dir_iter_next(OFSL_DirectoryIterator* it)
{
    return it->ops->dir_iter_next(it);
}

OFSL_INLINE
static inline void ofsl_dir_iter_end(OFSL_DirectoryIterator* it)
{
    it->ops->dir_iter_end(it);
}

OFSL_INLINE
static inline const char* ofsl_dir_iter_get_name(OFSL_DirectoryIterator* it)
{
    return it->ops->dir_iter_get_name(it);
}

OFSL_INLINE
static inline OFSL_FileType ofsl_dir_iter_get_type(OFSL_DirectoryIterator* it)
{
    return it->ops->dir_iter_get_type(it);
}

OFSL_INLINE
static inline int ofsl_dir_iter_get_timestamp(OFSL_DirectoryIterator* it, OFSL_TimestampType type, OFSL_Time* time)
{
    return it->ops->dir_iter_get_timestamp(it, type, time);
}

OFSL_INLINE
static inline int ofsl_dir_iter_get_attr(OFSL_DirectoryIterator* it, OFSL_FileAttributeType type)
{
    return it->ops->dir_iter_get_attr(it, type);
}

OFSL_INLINE
static inline int ofsl_dir_iter_get_size(OFSL_DirectoryIterator* it, size_t* size)
{
    return it->ops->dir_iter_get_size(it, size);
}

OFSL_INLINE
static inline int ofsl_file_create(OFSL_Directory* parent, const char* name)
{
    return parent->ops->file_create(parent, name);
}

OFSL_INLINE
static inline int ofsl_file_remove(OFSL_Directory* parent, const char* name)
{
    return parent->ops->file_remove(parent, name);
}

OFSL_INLINE
static inline OFSL_File* ofsl_file_open(OFSL_Directory* parent, const char* name, const char* mode)
{
    return parent->ops->file_open(parent, name, mode);
}

OFSL_INLINE
static inline int ofsl_file_close(OFSL_File* file)
{
    return file->ops->file_close(file);
}

OFSL_INLINE
static inline ssize_t ofsl_file_read(OFSL_File* file, void* buf, size_t size, size_t count)
{
    return file->ops->file_read(file, buf, size, count);
}

OFSL_INLINE
static inline ssize_t ofsl_file_write(OFSL_File* file, const void* buf, size_t size, size_t count)
{
    return file->ops->file_write(file, buf, size, count);
}

OFSL_INLINE
static inline int ofsl_file_seek(OFSL_File* file, ssize_t offset, int origin)
{
    return file->ops->file_seek(file, offset, origin);
}

OFSL_INLINE
static inline ssize_t ofsl_file_tell(OFSL_File* file)
{
    return file->ops->file_tell(file);
}

OFSL_INLINE
static inline int ofsl_file_iseof(OFSL_File* file)
{
    return file->ops->file_iseof(file);
}

#ifdef __cplusplus
};
#endif

#endif
