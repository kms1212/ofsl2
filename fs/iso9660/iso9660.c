#include <ofsl/fs/iso9660.h>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include <ofsl/drive/drive.h>
#include <ofsl/time.h>

#include "endian.h"
#include "fs/iso9660/internal.h"
#include "config.h"

#define DISKBUF_TYPE_SECTOR     0
#define DISKBUF_TYPE_CLUSTER    1

#define USAGE_TAG_MAX           32767

struct diskbuf_entry {
    uint16_t data_valid : 1;
    uint16_t usage_tag : 15;
    uint32_t lba;
    uint8_t data[];
};

struct fs_iso {
    OFSL_FileSystem fs;
    OFSL_Partition part;
    struct diskbuf_entry** diskbuf;
    uint8_t mounted : 1;
    uint32_t pathtbl_size;
    uint32_t volume_sector_count;
    uint16_t sector_size;
    uint32_t lba_primary_desc;
    uint32_t lba_pathtbl[2];

    struct ofsl_fs_iso9660_option options;
};

struct dir_iso {
    OFSL_Directory dir;
    struct dir_iso* parent;
    struct isofs_dir_entry_header direntry;
    uint32_t lba_data;
};

struct dirit_iso {
    OFSL_DirectoryIterator dirit;
    struct dir_iso* parent;
    uint32_t lba_current;
    uint16_t entry_pos_current;
    uint16_t prev_entry_size;
    int valid;
    struct isofs_dir_entry_header direntry;
    char filename[ISO9660_PATH_BUFSZ];
};

struct file_iso {
    OFSL_File file;
    struct dir_iso* parent;
    struct isofs_dir_entry_header direntry;
    size_t cursor;
    uint32_t lba_data;
};

static int match_name(struct dir_iso* parent, const char* name, struct isofs_dir_entry_header* direntry_buf);
static int file_iseof(OFSL_File* file_opaque);

static struct fs_iso* check_fs_mounted(OFSL_FileSystem* fs_opaque)
{
    struct fs_iso* fs = (struct fs_iso*)fs_opaque;
    if (fs == NULL) return NULL;
    if (!fs->mounted) {
        fs->fs.error = OFSL_FSE_UNMOUNTED;
        return NULL;
    }
    return fs;
}

static struct dir_iso* check_dir(OFSL_Directory* dir_opaque)
{
    return (struct dir_iso*)dir_opaque;
}

static struct file_iso* check_file(OFSL_File* file_opaque)
{
    return (struct file_iso*)file_opaque;
}

static size_t remove_right_padding(char* dest, const char* orig, size_t len)
{
    int str_len = 0;
    for (int cur = len - 1; cur >= 0; cur--) {
        if (str_len) {
            dest[cur] = orig[cur];
            str_len++;
        } else if (orig[cur] != ' ') {
            str_len++;
            if (cur < len - 1) {
                dest[cur] = orig[cur];
                dest[cur + 1] = 0;
            }
        }
    }
    return str_len;
}

static void get_longfmt_time(OFSL_Time* time, const struct isofs_time_longfmt* fstime)
{
    union {
        char astr[11];
        struct isofs_time_longfmt formatted;
    } cvt_buf = { .formatted = *fstime };

    for (int i = 0; i < 16; i++) {
        cvt_buf.astr[i] -= '0';
    }

    struct tm tm;
    tm.tm_sec  = (cvt_buf.formatted.second[0] * 10) + cvt_buf.formatted.second[1];
    tm.tm_min  = (cvt_buf.formatted.minute[0] * 10) + cvt_buf.formatted.minute[1];
    tm.tm_hour = (cvt_buf.formatted.hour[0] * 10) + cvt_buf.formatted.hour[1];
    tm.tm_mday = (cvt_buf.formatted.day[0] * 10) + cvt_buf.formatted.day[1];
    tm.tm_mon  = (cvt_buf.formatted.month[0] * 10) + cvt_buf.formatted.month[1] - 1;
    tm.tm_year = 0;
    for (int i = 0; i < 4; i++) {
        tm.tm_year *= 10;
        tm.tm_year += cvt_buf.formatted.year[i];
    }
    tm.tm_year -= 1900;

    ofsl_time_fromstdctm(time, &tm);

    time->nsec = (cvt_buf.formatted.ten_msec[0] * 10) + cvt_buf.formatted.ten_msec[1];
    time->nsec *= 10000000;  /* 10 * 1000 * 1000 */
}

static void get_shortfmt_time(OFSL_Time* time, const struct isofs_time_shortfmt* fstime)
{
    struct tm tm;
    tm.tm_isdst = 0;
    tm.tm_sec   = fstime->second;
    tm.tm_min   = fstime->minute;
    tm.tm_hour  = fstime->hour;
    tm.tm_mday  = fstime->day;
    tm.tm_mon   = fstime->month - 1;
    tm.tm_year  = fstime->year;
    ofsl_time_fromstdctm(time, &tm);
    time->nsec = 0;
}

/**
 * @brief Allocate diskbuf sector entry
 * 
 * @param fs filesystem object struct
 * @param entry_idx entry index output (NULL if not needed)
 * @param lba LBA address of the sector
 * @return int 0 if success, otherwise failed
 * 
 * @details
 *  This function finds, replaces, or allocates a diskbuf sector entry by the given lba value and
 * decreases the `usage_tag` member of each entries for further entry replacement.
 *  Flushes old entry when replacement is required.
 */
static int allocate_diskbuf_sector_entry(struct fs_iso* fs, unsigned int* entry_idx, uint32_t lba)
{
    int cached = 0;
    int target_entry_idx = -1;
    uint16_t usage_tag_min = USAGE_TAG_MAX;

    for (int i = 0; i < fs->options.diskbuf_count; i++) {
        if (fs->diskbuf[i] == NULL) {
            if (target_entry_idx < 0 && !cached) {
                target_entry_idx = i;
            }
        } else {
            if (fs->diskbuf[i]->lba == lba) {
                target_entry_idx = i;
                cached = 1;
            } else if (
                (fs->diskbuf[i]->usage_tag < usage_tag_min) &&
                !cached &&
                (target_entry_idx < 0)) {
                usage_tag_min = fs->diskbuf[i]->usage_tag;
                target_entry_idx = i;
            }

            if (fs->diskbuf[i]->usage_tag > 0) {
                fs->diskbuf[i]->usage_tag--;
            }
        } 
    }

    if (!cached) {
        if (!fs->diskbuf[target_entry_idx]) {
            fs->diskbuf[target_entry_idx] = malloc(sizeof(struct diskbuf_entry) + fs->sector_size);
        }

        fs->diskbuf[target_entry_idx]->data_valid = 0;
        fs->diskbuf[target_entry_idx]->lba = lba;
    }
    fs->diskbuf[target_entry_idx]->usage_tag = USAGE_TAG_MAX;

    *entry_idx = target_entry_idx;
    return 0;
}

/**
 * @brief Read a sector from disk and write into a diskbuf entry
 * 
 * @param fs filesystem object struct
 * @param entry_idx entry index output (NULL if not needed)
 * @param lba LBA address of the sector
 * @return int 0 if success, otherwise failed
 */
static int read_sector(struct fs_iso* fs, unsigned int* entry_idx, lba_t lba)
{
    unsigned int target_entry_idx;
    allocate_diskbuf_sector_entry(fs, &target_entry_idx, lba);

    if (!fs->diskbuf[target_entry_idx]->data_valid) {
        ofsl_drive_read_sector(
            fs->part.drv,
            fs->diskbuf[target_entry_idx]->data,
            fs->part.lba_start + lba,
            fs->sector_size,
            1);
        fs->diskbuf[target_entry_idx]->data_valid = 1;
    }

    if (entry_idx) {
        *entry_idx = target_entry_idx;
    }
    return 0;
}

static int mount(OFSL_FileSystem* fs_opaque)
{
    struct fs_iso* fs = (struct fs_iso*)fs_opaque;

    fs->diskbuf = calloc(fs->options.diskbuf_count, sizeof(struct diskbuf_entry*));
    fs->sector_size = 2048;

    lba_t lba_current_descriptor = 16;
    struct isofs_vol_desc* voldesc;
    unsigned int entry_idx;

    // find primary volume descriptor
    fs->lba_primary_desc = 0;
    do {
        read_sector(fs, &entry_idx, lba_current_descriptor);
        voldesc = (void*)fs->diskbuf[entry_idx]->data;

        /* check signature */
        if (strncmp(voldesc->signature, ISO9660_SIGNATURE, 5) != 0) return 1;

        switch (voldesc->type) {
            case VDTYPE_PRIVOLDESC:
                if (!fs->lba_primary_desc) {
                    fs->lba_primary_desc = lba_current_descriptor;
                }
                break;
        }
        lba_current_descriptor++;
    } while (voldesc->type != VDTYPE_VDSETTERM);

    if (!fs->lba_primary_desc) return 1;

    read_sector(fs, &entry_idx, fs->lba_primary_desc);
    voldesc = (void*)fs->diskbuf[entry_idx]->data;

#ifdef BYTE_ORDER_BIG_ENDIAN
    fs->lba_pathtbl[0] = voldesc->primary_vol_desc.lba_be_pathtbl;
    fs->lba_pathtbl[1] = voldesc->primary_vol_desc.lba_be_pathtbl_optional;

#else
    fs->lba_pathtbl[0] = voldesc->primary_vol_desc.lba_le_pathtbl;
    fs->lba_pathtbl[1] = voldesc->primary_vol_desc.lba_le_pathtbl_optional;

#endif
    fs->pathtbl_size = get_biendian_value(&voldesc->primary_vol_desc.pathtbl_size);

    fs->sector_size = get_biendian_value(&voldesc->primary_vol_desc.sector_size);
    fs->volume_sector_count = get_biendian_value(&voldesc->primary_vol_desc.vol_sector_count);

    fs->mounted = 1;

    return 0;
}

static int unmount(OFSL_FileSystem* fs_opaque)
{
    struct fs_iso* fs = check_fs_mounted(fs_opaque);
    if (!fs) return 1;

    fs->mounted = 0;
    return 0;
}

static void _delete(OFSL_FileSystem* fs_opaque)
{
    struct fs_iso* fs = (struct fs_iso*)fs_opaque;

    free(fs);
}

static OFSL_Directory* rootdir_open(OFSL_FileSystem* fs_opaque)
{
    struct fs_iso* fs = check_fs_mounted(fs_opaque);
    struct dir_iso* dir = malloc(sizeof(struct dir_iso));
    unsigned int entry_idx;

    dir->dir.fs = &fs->fs;
    dir->dir.ops = fs->fs.ops;

    read_sector(fs, &entry_idx, fs->lba_pathtbl[0]);
    struct isofs_pathtbl_entry_header* pathtbl_entry = (void*)fs->diskbuf[entry_idx]->data;
    dir->lba_data = pathtbl_entry->lba_data;

    read_sector(fs, &entry_idx, fs->lba_primary_desc);
    struct isofs_vol_desc* voldesc = (void*)fs->diskbuf[entry_idx]->data;
    dir->parent = NULL;
    memcpy(
        &dir->direntry,
        &voldesc->primary_vol_desc.rootdir_entry_header,
        sizeof(struct isofs_dir_entry_header));

    return (OFSL_Directory*)dir;
}

static OFSL_Directory* dir_open(OFSL_Directory* parent_opaque, const char* name)
{
    struct dir_iso* parent = check_dir(parent_opaque);
    if (!parent) return NULL;
    struct fs_iso* fs = check_fs_mounted(parent->dir.fs);
    if (!fs) return NULL;

    struct isofs_dir_entry_header dirent;
    if (!match_name(parent, name, &dirent)) {
        fs->fs.error = OFSL_FSE_NOENT;
        return  NULL;
    }

    struct dir_iso* dir = malloc(sizeof(struct dir_iso));
    dir->dir.ops = fs->fs.ops;
    dir->dir.fs = parent->dir.fs;
    dir->parent = parent;
    dir->lba_data = get_biendian_value(&dirent.lba_data_location);
    memcpy(&dir->direntry, &dirent, sizeof(dirent));

    return (OFSL_Directory*)dir;
}

static int dir_close(OFSL_Directory* dir_opaque)
{
    struct dir_iso* dir = check_dir(dir_opaque);
    if (!dir) return 1;

    free(dir);
    return 0;
}

static OFSL_DirectoryIterator* dir_iter_start(OFSL_Directory* dir_opaque)
{
    struct dir_iso* dir = check_dir(dir_opaque);
    if (!dir) return NULL;
    struct fs_iso* fs = check_fs_mounted(dir->dir.fs);
    if (!fs) return NULL;

    struct dirit_iso* it = malloc(sizeof(struct dirit_iso));
    it->dirit.fs = dir->dir.fs;
    it->dirit.ops = fs->fs.ops;
    it->parent = dir;
    it->valid = 0;
    it->lba_current = dir->lba_data;
    it->entry_pos_current = 0;
    
    return (OFSL_DirectoryIterator*)it;
}

static int dir_iter_next(OFSL_DirectoryIterator* it_opaque)
{
    struct dirit_iso* it = (struct dirit_iso*)it_opaque;
    if (!it) return 1;
    struct dir_iso* dir = it->parent;
    if (!dir) return 1;
    struct fs_iso* fs = check_fs_mounted(dir->dir.fs);
    if (!fs) return 1;

    unsigned int entry_idx;

    read_sector(fs, &entry_idx, it->lba_current);
    struct isofs_dir_entry_header* direnthdr = (void*)((uint8_t*)fs->diskbuf[entry_idx]->data + it->entry_pos_current);
    if (!direnthdr->entry_size) return 1;

    it->prev_entry_size = direnthdr->entry_size + direnthdr->attrib_record_size;

    it->entry_pos_current += direnthdr->entry_size + direnthdr->attrib_record_size;
    if (it->entry_pos_current >= fs->sector_size) {
        it->entry_pos_current = 0;
        it->lba_current++;
    }

    char* filename = (char*)direnthdr + sizeof(*direnthdr);
    if (direnthdr->filename_len > 1 || filename[0] > 2) {
        strncpy(it->filename, filename, direnthdr->filename_len);
        it->filename[direnthdr->filename_len] = 0;
    } else if (!filename[0]) {
        strncpy(it->filename, "..", 3);
    } else {
        strncpy(it->filename, ".", 2);
    }


#if defined(BUILD_SYSTEM_ISO9660_ROCKRIDGE) || \
    defined(BUILD_FILESYSTEM_ISO9660_JOILET)
    // directory entry extensions
    uint16_t extension_entry_cur = sizeof(*direnthdr) + direnthdr->filename_len;
    extension_entry_cur += direnthdr->filename_len & 1 ? 0 : 1;

    while (extension_entry_cur < direnthdr->entry_size) {
        struct isofs_dir_entry_extension_header* exthdr = (void*)((uint8_t*)direnthdr + extension_entry_cur);
        if (!exthdr->entry_len) break;
        extension_entry_cur += exthdr->entry_len;

#ifdef BUILD_FILESYSTEM_ISO9660_ROCKRIDGE
        if (fs->options.enable_rock_ridge) {
            if (exthdr->identifier[0] == 'N' && exthdr->identifier[1] == 'M') {
                struct rrip_nm_entry* nm_entry = (struct rrip_nm_entry*)exthdr;
                if (nm_entry->current_directory) {
                    strncpy(it->filename, ".", 2);
                } else if (nm_entry->parent_directory) {
                    strncpy(it->filename, "..", 3);
                } else {
                    strncpy(it->filename, nm_entry->filename, nm_entry->header.entry_len - 5);
                    it->filename[nm_entry->header.entry_len - 5] = 0;
                }
                break;
            }
        }

#endif
    }

#endif

    /* copy directory entry header */
    memcpy(&it->direntry, direnthdr, sizeof(*direnthdr));

    it->valid = 1;

    return 0;
}

static void dir_iter_end(OFSL_DirectoryIterator* it_opaque)
{
    free(it_opaque);
}

static const char* dir_iter_get_name(OFSL_DirectoryIterator* it_opaque)
{
    struct dirit_iso* it = (struct dirit_iso*)it_opaque;
    if (!it) return NULL;

    return it->valid ? it->filename : NULL;
}

static OFSL_FileType dir_iter_get_type(OFSL_DirectoryIterator* it_opaque)
{
    struct dirit_iso* it = (struct dirit_iso*)it_opaque;
    if (!it) return OFSL_FTYPE_ERROR;

    return it->direntry.directory ? OFSL_FTYPE_DIR : OFSL_FTYPE_FILE;
}

static int dir_iter_get_timestamp(OFSL_DirectoryIterator* it_opaque, OFSL_TimestampType type, OFSL_Time* time)
{
    struct dirit_iso* it = (struct dirit_iso*)it_opaque;
    if (!it) return 1;
    struct dir_iso* dir = it->parent;
    if (!dir) return 1;
    struct fs_iso* fs = check_fs_mounted(dir->dir.fs);
    if (!fs) return 1;

    struct tm tm;

#if defined(BUILD_SYSTEM_ISO9660_ROCKRIDGE) || \
    defined(BUILD_FILESYSTEM_ISO9660_JOILET)
    unsigned int entry_idx;

    read_sector(fs, &entry_idx, it->lba_current);
    struct isofs_dir_entry_header* direnthdr = (void*)((uint8_t*)fs->diskbuf[entry_idx]->data + it->entry_pos_current - it->prev_entry_size);
    if (!direnthdr->entry_size) return 1;

    // directory entry extensions
    uint16_t extension_entry_cur = sizeof(*direnthdr) + direnthdr->filename_len;
    extension_entry_cur += direnthdr->filename_len & 1 ? 0 : 1;

    while (extension_entry_cur < direnthdr->entry_size) {
        struct isofs_dir_entry_extension_header* exthdr = (void*)((uint8_t*)direnthdr + extension_entry_cur);
        if (!exthdr->entry_len) break;
        extension_entry_cur += exthdr->entry_len;

#ifdef BUILD_FILESYSTEM_ISO9660_ROCKRIDGE
        if (fs->options.enable_rock_ridge) {
            if (exthdr->identifier[0] == 'T' && exthdr->identifier[1] == 'F') {
                const static OFSL_TimestampType ftlut[] = {
                    OFSL_TSTYPE_CREATION,
                    OFSL_TSTYPE_MODIFICATION,
                    OFSL_TSTYPE_ACCESS,
                    OFSL_TSTYPE_ATTR_MODIFICATION,
                    OFSL_TSTYPE_BACKUP,
                    OFSL_TSTYPE_EXPIRATION,
                    OFSL_TSTYPE_EFFECTIVE,
                };

                struct rrip_tf_entry* tf_entry = (struct rrip_tf_entry*)exthdr;
                uint8_t ts_offset = 5;
                uint8_t flags_temp = tf_entry->flags_raw;

                for (int i = 0; i < 7; i++) {
                    if (flags_temp & 1) {
                        if (ftlut[i] == type) {
                            if (tf_entry->long_format) {
                                struct isofs_time_longfmt* ts = (void*)((uint8_t*)tf_entry + ts_offset);
                                get_longfmt_time(time, ts);
                            } else {
                                struct isofs_time_shortfmt* ts = (void*)((uint8_t*)tf_entry + ts_offset);
                                get_shortfmt_time(time, ts);
                            }
                            return 0;
                        }
                        ts_offset += tf_entry->long_format ? 17 : 7;
                    }

                    flags_temp >>= 1;
                }

                break;
            }
        }

#endif
    }

#endif

    switch (type) {
        case OFSL_TSTYPE_CREATION:
            get_shortfmt_time(time, &it->direntry.created_time);
            break;
        default:
            return 1;
    }

    return 0;
}

static int dir_iter_get_attr(OFSL_DirectoryIterator* it_opaque, OFSL_FileAttributeType type)
{
    struct dirit_iso* it = (struct dirit_iso*)it_opaque;
    if (!it) return -1;

    return -1;
}

static int dir_iter_get_size(OFSL_DirectoryIterator* it_opaque, size_t* size)
{
    struct dirit_iso* it = (struct dirit_iso*)it_opaque;
    if (!it) return 1;

    *size = get_biendian_value(&it->direntry.data_size);
    return 0;
}

static const char* get_fs_name(OFSL_FileSystem* fs_opaque)
{
    struct fs_iso* fs = check_fs_mounted(fs_opaque);
    if (!fs) return NULL;

    return "ISO9660";
}

static OFSL_File* file_open(OFSL_Directory* parent_opaque, const char* name, const char* mode)
{
    struct dir_iso* parent = check_dir(parent_opaque);
    if (!parent) return NULL;
    struct fs_iso* fs = check_fs_mounted(parent->dir.fs);
    if (!fs) return NULL;

    struct isofs_dir_entry_header dirent;
    if (!match_name(parent, name, &dirent)) {
        fs->fs.error = OFSL_FSE_NOENT;
        return NULL;
    }

    struct file_iso* file = malloc(sizeof(struct file_iso));
    file->file.ops = fs->fs.ops;
    file->file.fs = parent->dir.fs;
    file->lba_data = get_biendian_value(&dirent.lba_data_location);
    file->parent = parent;
    file->cursor = 0;
    memcpy(&file->direntry, &dirent, sizeof(dirent));

    return (OFSL_File*)file;
}

static int file_close(OFSL_File* file_opaque)
{
    struct file_iso* file = check_file(file_opaque);
    if (!file) return 1;
    struct fs_iso* fs = check_fs_mounted(file->file.fs);
    if (!fs) return 1;

    free(file);
    return 0;
}

static ssize_t file_read(OFSL_File* file_opaque, void* buf, size_t size, size_t count)
{
    struct file_iso* file = check_file(file_opaque);
    if (!file) return 1;
    struct fs_iso* fs = check_fs_mounted(file->file.fs);
    if (!fs) return 1;

    if (file_iseof((OFSL_File*)file)) return -1;

    uint8_t* bbuf = buf;
    uint32_t lba_current = file->lba_data + file->cursor / fs->sector_size;
    unsigned int entry_idx;

    for (size_t blkcnt = 0; blkcnt < count; blkcnt++) {
        if (file->cursor + size > get_biendian_value(&file->direntry.data_size)) {
            return blkcnt;
        }
        uint32_t sector_offs = file->cursor & fs->sector_size;
        size_t block_read_bytes = 0;

        for (;;) {
            uint16_t sector_max_read = fs->sector_size - sector_offs;
            uint16_t block_max_read = size - block_read_bytes;

            read_sector(fs, &entry_idx, lba_current);

            if (sector_max_read > block_max_read) {
                memcpy(bbuf, fs->diskbuf[entry_idx]->data + sector_offs, block_max_read);
                block_read_bytes += block_max_read;
                bbuf += block_max_read;
                file->cursor += block_max_read;
                break;
            } else {
                memcpy(bbuf, fs->diskbuf[entry_idx]->data + sector_offs, sector_max_read);
                block_max_read += sector_max_read;
                bbuf += sector_max_read;
                file->cursor += sector_max_read;
                lba_current++;
            }
        }
    }

    return count;
}

static int file_seek(OFSL_File* file_opaque, ssize_t offset, int origin)
{
    struct file_iso* file = check_file(file_opaque);
    if (!file) return 1;

    switch (origin) {
        case SEEK_SET:
            if ((offset > get_biendian_value(&file->direntry.data_size)) ||
                (offset < 0)) return 1;
            file->cursor = offset;
            break;
        case SEEK_CUR:
            if ((offset + file->cursor > get_biendian_value(&file->direntry.data_size)) ||
                (offset + file->cursor < 0)) return 1;
            file->cursor += offset;
            break;
        case SEEK_END:
            if ((offset > 0) ||
                (offset + get_biendian_value(&file->direntry.data_size) < 0)) return 1;
            file->cursor = get_biendian_value(&file->direntry.data_size) + offset;
            break;
        default:
            return 1;
    }
    return 0;
}

static ssize_t file_tell(OFSL_File* file_opaque)
{
    struct file_iso* file = check_file(file_opaque);
    if (!file) return -1;

    if (file->cursor < 0 || file->cursor > get_biendian_value(&file->direntry.data_size)) return -1;
    return file->cursor;
}

static int file_iseof(OFSL_File* file_opaque)
{
    struct file_iso* file = check_file(file_opaque);
    if (!file) return -1;
    return file->cursor >= get_biendian_value(&file->direntry.data_size);
}

static int get_volume_string(OFSL_FileSystem* fs_opaque, OFSL_VolumeStringType type, char* buf, size_t len)
{
    struct fs_iso* fs = check_fs_mounted(fs_opaque);
    if (!fs) return 1;
    
    unsigned int entry_idx;
    read_sector(fs, &entry_idx, fs->lba_primary_desc);
    struct isofs_vol_desc* voldesc = (void*)fs->diskbuf[entry_idx]->data;

    /* replace literals into macro or sizeof()*/
    switch (type) {
        case OFSL_VSTYPE_LABEL:
            remove_right_padding(buf, voldesc->primary_vol_desc.vol_name, len < 32 ? len : 32);
            break;
        case OFSL_VSTYPE_PUBLISHER:
            remove_right_padding(buf, voldesc->primary_vol_desc.publisher_iden, len < 128 ? len : 128);
            break;
        case OFSL_VSTYPE_AUTHOR:
            remove_right_padding(buf, voldesc->primary_vol_desc.data_author_iden, len < 128 ? len : 128);
            break;
        case OFSL_VSTYPE_PROGRAM:
            remove_right_padding(buf, voldesc->primary_vol_desc.application_iden, len < 128 ? len : 128);
            break;
        case OFSL_VSTYPE_COPYRIGHT:
            remove_right_padding(buf, voldesc->primary_vol_desc.copyright_file_name, len < 37 ? len : 37);
            break;
        case OFSL_VSTYPE_ABSTRACT:
            remove_right_padding(buf, voldesc->primary_vol_desc.abstract_file_name, len < 37 ? len : 37);
            break;
        case OFSL_VSTYPE_BIBLIOGRAPHY:
            remove_right_padding(buf, voldesc->primary_vol_desc.bibliographic_file_name, len < 37 ? len : 37);
            break;
        default:
            return 1;
    }
    return 0;
}

static int get_volume_timestamp(OFSL_FileSystem* fs_opaque, OFSL_TimestampType type, OFSL_Time* time)
{
    struct fs_iso* fs = check_fs_mounted(fs_opaque);
    if (!fs) return 1;
    
    unsigned int entry_idx;
    read_sector(fs, &entry_idx, fs->lba_primary_desc);
    struct isofs_vol_desc* voldesc = (void*)fs->diskbuf[entry_idx]->data;

    switch (type) {
        case OFSL_TSTYPE_CREATION:
            get_longfmt_time(time, &voldesc->primary_vol_desc.time_created);
            break;
        case OFSL_TSTYPE_MODIFICATION:
            get_longfmt_time(time, &voldesc->primary_vol_desc.time_modified);
            break;
        case OFSL_TSTYPE_EXPIRATION:
            get_longfmt_time(time, &voldesc->primary_vol_desc.time_expired_after);
            break;
        case OFSL_TSTYPE_EFFECTIVE:
            get_longfmt_time(time, &voldesc->primary_vol_desc.time_effective_after);
            break;
        default:
            return 1;
    }

    return 0;
}

static int match_name(struct dir_iso* parent, const char* name, struct isofs_dir_entry_header* direntry_buf)
{
    if (!parent) return 0;
    struct fs_iso* fs = check_fs_mounted(parent->dir.fs);

    struct dirit_iso* it = (struct dirit_iso*)dir_iter_start((OFSL_Directory*)parent);
    while (!dir_iter_next((OFSL_DirectoryIterator*)it)) {
        if (fs->options.case_sensitive) {
            if (strncmp(name, it->filename, sizeof(it->filename)) == 0) {
                memcpy(direntry_buf, &it->direntry, sizeof(*direntry_buf));
                dir_iter_end((OFSL_DirectoryIterator*)it);
                return 1;
            }
        } else {
            if (strncasecmp(name, it->filename, sizeof(it->filename)) == 0) {
                memcpy(direntry_buf, &it->direntry, sizeof(*direntry_buf));
                dir_iter_end((OFSL_DirectoryIterator*)it);
                return 1;
            }
        }
    }
    dir_iter_end((OFSL_DirectoryIterator*)it);
    return 0;
}

OFSL_EXPORT
OFSL_FileSystem* ofsl_fs_iso9660_create(OFSL_Partition* part)
{
    static const struct ofsl_fs_ops fsops = {
        //.get_error_string = get_error_string,
        ._delete = _delete,
        .mount = mount,
        .unmount = unmount,
        .get_fs_name = get_fs_name,
        .get_volume_string = get_volume_string,
        .get_volume_timestamp = get_volume_timestamp,
        //.dir_create = dir_create,
        //.dir_remove = dir_remove,
        .rootdir_open = rootdir_open,
        .dir_open = dir_open,
        .dir_close = dir_close,
        .dir_iter_start = dir_iter_start,
        .dir_iter_next = dir_iter_next,
        .dir_iter_end = dir_iter_end,
        .dir_iter_get_name = dir_iter_get_name,
        .dir_iter_get_type = dir_iter_get_type,
        .dir_iter_get_timestamp = dir_iter_get_timestamp,
        .dir_iter_get_attr = dir_iter_get_attr,
        .dir_iter_get_size = dir_iter_get_size,
        //.file_create = file_create,
        //.file_remove = file_remove,
        .file_open = file_open,
        .file_close = file_close,
        .file_read = file_read,
        //.file_write = file_write,
        .file_seek = file_seek,
        .file_tell = file_tell,
        .file_iseof = file_iseof,
    };//

    if (part->drv->drvinfo.sector_size < 2048) {
        return NULL;
    }

    struct fs_iso* fs = malloc(sizeof(struct fs_iso));
    
    fs->fs.ops = &fsops;
    fs->part = *part;

    fs->mounted = 0;

    fs->options.diskbuf_count = 32;
    fs->options.enable_joilet = 1;
    fs->options.enable_rock_ridge = 1;

    return (OFSL_FileSystem*)fs;
}
