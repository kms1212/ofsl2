#include <ofsl/fs/fat.h>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include <ofsl/drive/drive.h>
#include <ofsl/time.h>

#include "src/endian.h"
#include "fs/fat/internal.h"
#include "fs/fat/defaults.h"
#include "fs/internal.h"

#define DISKBUF_TYPE_SECTOR     0
#define DISKBUF_TYPE_CLUSTER    1

#define USAGE_TAG_MAX           8191

enum error_fat {
    FATE_IDBENT = -1,
};

static const char* error_str_list[] = {
    "Invalid disk buffer entry",
};

struct diskbuf_entry {
    uint16_t type : 1;
    uint16_t dirty : 1;
    uint16_t data_valid : 1;
    uint16_t usage_tag : 13;
    union {
        fatcluster_t cluster;
        lba_t lba;
    };
    uint8_t data[];
};

struct fs_fat {
    OFSL_FileSystem fs;
    OFSL_Partition part;
    struct diskbuf_entry** diskbuf;
    uint16_t    reserved_sectors;
    uint16_t    sector_size;
    uint32_t    cluster_size;
    uint8_t     sectors_per_cluster;
    uint8_t     fat_type : 2;
    uint8_t     mounted : 1;
    uint32_t    data_area_begin;
    uint32_t    fat_size;
    uint32_t    free_clusters;
    uint32_t    next_free_cluster;
    uint32_t    total_sector_count;
    uint32_t    root_cluster;
    uint16_t    root_entry_count;
    uint16_t    root_sector_count;
    struct ofsl_fs_fat_option options;
};

struct file_fat {
    OFSL_File file;
    uint32_t head_cluster;
    struct dir_fat* parent;
    uint32_t direntry_cluster_idx;
    uint8_t direntry_idx;
    struct fat_direntry_file direntry;
    uint32_t cursor;
};

struct dir_fat {
    OFSL_Directory dir;
    uint32_t head_cluster;
    struct dir_fat* parent;
    uint32_t child_count;
    struct fat_direntry_file direntry;
};

struct dirit_fat {
    OFSL_DirectoryIterator dirit;
    struct dir_fat* parent;
    uint32_t current_cluster_idx;
    uint8_t current_entry_idx;
    int valid;
    char filename[FAT_LFN_U8_BUFLEN];
    struct fat_direntry_file direntry;
};

static int read_fat(struct fs_fat* fs, unsigned int* entry_idx, uint32_t sector_idx);
static int file_iseof(OFSL_File* file_opaque);
static int match_name(OFSL_Directory* parent_opaque, const char* name, struct fat_direntry_file* direntry_buf);

static struct fs_fat* check_fs_mounted(OFSL_FileSystem* fs_opaque)
{
    struct fs_fat* fs = (struct fs_fat*)fs_opaque;
    if (fs == NULL) {
        return NULL;
    } else if (!fs->mounted) {
        fs->fs.error = OFSL_FSE_UNMOUNTED;
        return NULL;
    }
    return fs;
}

static struct dir_fat* check_dir(OFSL_Directory* dir_opaque)
{
    return (struct dir_fat*)dir_opaque;
}

static struct file_fat* check_file(OFSL_File* file_opaque)
{
    return (struct file_fat*)file_opaque;
}

static int sector_to_cluster(struct fs_fat* fs, fatcluster_t* cluster, lba_t lba) {
    switch (fs->fat_type) {
        case FAT_TYPE_FAT12:
        case FAT_TYPE_FAT16:
            *cluster = (lba - fs->data_area_begin - fs->root_sector_count) / fs->sectors_per_cluster + 2;
            return 0;
        case FAT_TYPE_FAT32:
            *cluster = (lba - fs->data_area_begin) / fs->sectors_per_cluster + 2;
            return 0;
        default:
            fs->fs.error = OFSL_FSE_INVALFS;
            return 1;
    }
}

static int cluster_to_sector(struct fs_fat* fs, lba_t* lba, fatcluster_t cluster) {
    switch (fs->fat_type) {
        case FAT_TYPE_FAT12:
        case FAT_TYPE_FAT16:
            *lba = ((cluster - 2) * fs->sectors_per_cluster) + fs->data_area_begin + fs->root_sector_count;
            return 0;
        case FAT_TYPE_FAT32:
            *lba = ((cluster - 2) * fs->sectors_per_cluster) + fs->data_area_begin;
            return 0;
        default:
            fs->fs.error = OFSL_FSE_INVALFS;
            return 1;
    }
}

static int get_next_cluster(struct fs_fat* fs, fatcluster_t* cluster, uint32_t num)
{
    switch (fs->fat_type) {
        case FAT_TYPE_FAT12:
            while (num-- > 0) {
                if (*cluster > FAT12_MAX_CLUSTER) {
                    fs->fs.error = OFSL_FSE_ICLUSTER;
                    return 1;
                }

                uint16_t byte_idx = *cluster + (*cluster >> 1);
                uint16_t sector_idx = byte_idx / fs->sector_size;
                byte_idx %= fs->sector_size;

                unsigned int entry_idx;
                read_fat(fs, &entry_idx, sector_idx);

                uint8_t fatentry_buf[2];

                fatentry_buf[0] = fs->diskbuf[entry_idx]->data[byte_idx];
                if (byte_idx == fs->sector_size - 1) {
                    unsigned int entry_idx;
                    read_fat(fs, &entry_idx, sector_idx + 1);
                    fatentry_buf[1] = fs->diskbuf[entry_idx]->data[0];
                } else {
                    fatentry_buf[1] = fs->diskbuf[entry_idx]->data[byte_idx + 1];
                }

                if (*cluster & 1) {  // odd-numbered cluster
                    *cluster = ((fatentry_buf[0] & 0xF0) >> 4) | (fatentry_buf[1] << 4);
                } else {  // even-numbered cluster
                    *cluster = fatentry_buf[0] | ((fatentry_buf[1] & 0x0F) << 8);
                }

                if (*cluster > FAT16_MAX_CLUSTER) {
                    return 1;
                }
            }
            break;
        case FAT_TYPE_FAT16:
            while (num-- > 0) {
                if (*cluster > FAT16_MAX_CLUSTER) {
                    fs->fs.error = OFSL_FSE_ICLUSTER;
                    return 1;
                }

                uint32_t fatentry_idx = *cluster;
                uint32_t sector_idx = fatentry_idx / (fs->sector_size >> 1);
                fatentry_idx %= fs->sector_size >> 1;

                unsigned int entry_idx;
                read_fat(fs, &entry_idx, sector_idx);

                *cluster = ((uint16_t*)fs->diskbuf[entry_idx]->data)[fatentry_idx];
                if (*cluster > FAT16_MAX_CLUSTER) {
                    return 1;
                }
            }
            break;
        case FAT_TYPE_FAT32:
            while (num-- > 0) {
                if (*cluster > FAT32_MAX_CLUSTER) {
                    fs->fs.error = OFSL_FSE_ICLUSTER;
                    return 1;
                }
                uint32_t fatentry_idx = *cluster;
                uint32_t sector_idx = fatentry_idx / (fs->sector_size >> 2);
                fatentry_idx %= fs->sector_size >> 2;

                unsigned int entry_idx;
                read_fat(fs, &entry_idx, sector_idx);

                *cluster = ((uint32_t*)fs->diskbuf[entry_idx]->data)[fatentry_idx];
                if (*cluster > FAT32_MAX_CLUSTER) {
                    return 1;
                }
            }
            break;
        default:
            fs->fs.error = OFSL_FSE_INVALFS;
            return 1;
    }
    return 0;
}

static int flush_diskbuf_entry(struct fs_fat* fs, unsigned int entry) {
    if (!fs->diskbuf[entry]) {
        fs->fs.error = FATE_IDBENT;
        return 1;
    }

    if (fs->diskbuf[entry]->dirty) {
        switch (fs->diskbuf[entry]->type) {
            case DISKBUF_TYPE_CLUSTER: {
                lba_t clus_head_lba;
                cluster_to_sector(fs, &clus_head_lba, fs->diskbuf[entry]->cluster);
                ofsl_drive_write_sector(
                    fs->part.drv,
                    fs->diskbuf[entry]->data,
                    fs->part.lba_start + clus_head_lba,
                    fs->sector_size,
                    fs->sectors_per_cluster);
                fs->diskbuf[entry]->dirty = 0;
                break;
            }
            case DISKBUF_TYPE_SECTOR:
                ofsl_drive_write_sector(
                    fs->part.drv,
                    fs->diskbuf[entry]->data,
                    fs->part.lba_start + fs->diskbuf[entry]->lba,
                    fs->sector_size,
                    1);
                fs->diskbuf[entry]->dirty = 0;
                break;
        }
    }

    fs->diskbuf[entry]->usage_tag = USAGE_TAG_MAX;

    return 0;
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
static int allocate_diskbuf_sector_entry(struct fs_fat* fs, unsigned int* entry_idx, lba_t lba)
{
#define SHOULD_REALLOC  0
#define SHOULD_ALLOC    1
#define CACHED          2
    unsigned int status = SHOULD_REALLOC;
    int target_entry_idx = -1;
    uint16_t usage_tag_min = USAGE_TAG_MAX;

    for (int i = 0; i < fs->options.diskbuf_count; i++) {
        if (fs->diskbuf[i] == NULL) {
            if (status < SHOULD_ALLOC) {
                target_entry_idx = i;
                status = SHOULD_ALLOC;
            }
        } else {
            if ((fs->diskbuf[i]->type == DISKBUF_TYPE_SECTOR) &&
                (fs->diskbuf[i]->lba == lba)) {
                target_entry_idx = i;
                status = CACHED;
            } else if (
                (fs->diskbuf[i]->usage_tag < usage_tag_min) &&
                (status == SHOULD_REALLOC)) {
                usage_tag_min = fs->diskbuf[i]->usage_tag;
                target_entry_idx = i;
            }

            if (fs->diskbuf[i]->usage_tag > 0) {
                fs->diskbuf[i]->usage_tag--;
            }
        } 
    }

    if (status < CACHED) {
        if (status == SHOULD_REALLOC) {
            flush_diskbuf_entry(fs, target_entry_idx);
            if (fs->diskbuf[target_entry_idx]->type != DISKBUF_TYPE_SECTOR) {
                fs->diskbuf[target_entry_idx] = realloc(
                    fs->diskbuf[target_entry_idx],
                    sizeof(struct diskbuf_entry) + fs->sector_size);
            }
        } else {
            fs->diskbuf[target_entry_idx] = malloc(sizeof(struct diskbuf_entry) + fs->sector_size);
        }

        fs->diskbuf[target_entry_idx]->type = DISKBUF_TYPE_SECTOR;
        fs->diskbuf[target_entry_idx]->dirty = 0;
        fs->diskbuf[target_entry_idx]->data_valid = 0;
        fs->diskbuf[target_entry_idx]->lba = lba;
    }
    fs->diskbuf[target_entry_idx]->usage_tag = USAGE_TAG_MAX;

    *entry_idx = target_entry_idx;
    return 0;
#undef SHOULD_REALLOC
#undef SHOULD_ALLOC
#undef CACHED
}

/**
 * @brief Read a sector from disk and write into a diskbuf entry
 * 
 * @param fs filesystem object struct
 * @param entry_idx entry index output (NULL if not needed)
 * @param lba LBA address of the sector
 * @return int 0 if success, otherwise failed
 */
static int read_sector(struct fs_fat* fs, unsigned int* entry_idx, lba_t lba)
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

/**
 * @brief Overwrite a sector from the data of the given buffer.
 * 
 * @param fs filesystem object struct
 * @param entry_idx  entry index output (NULL if not needed)
 * @param buf sector data buffer. The size of the buffer should equal or greater than the size of the sector.
 * @param lba LBA address of the sector
 * @return int int 0 if success, otherwise failed
 * 
 * @details
 * This function overwrites entire sector data with the data of the given buffer.
 * If you want to make changes in smaller units, follow these methods:
 * - Call read_sector() with desired lba and get entry index.
 * - Make changes to the data of the entry and set the dirty bit. e.g.
 *   `fs->diskbuf[entry_idx]->dirty = 1`
 * - The changes will be written to the disk when the entry is flushed.
 */
static int write_sector(struct fs_fat* fs, unsigned int* entry_idx, const void* buf, lba_t lba)
{
    unsigned int target_entry_idx;
    allocate_diskbuf_sector_entry(fs, &target_entry_idx, lba);

    memcpy(fs->diskbuf[target_entry_idx]->data, buf, fs->sector_size);
    fs->diskbuf[target_entry_idx]->dirty = 1;

    if (entry_idx) {
        *entry_idx = target_entry_idx;
    }
    return 0;
}

/**
 * @brief Allocate diskbuf cluster entry
 * 
 * @param fs filesystem object struct
 * @param entry_idx entry index output (NULL if not needed)
 * @param cluster cluster index
 * @return int 0 if success, otherwise failed
 * 
 * @details
 *  This function finds, replaces, or allocates a diskbuf cluster entry by the given lba value and
 * decreases the `usage_tag` member of each entries for further entry replacement.
 *  Flushes old entry when replacement is required.
 */
static int allocate_diskbuf_cluster_entry(struct fs_fat* fs, unsigned int* entry_idx, fatcluster_t cluster)
{
#define SHOULD_REALLOC  0
#define SHOULD_ALLOC    1
#define CACHED          2
    unsigned int status = SHOULD_REALLOC;
    int target_entry_idx = -1;
    uint16_t usage_tag_min = USAGE_TAG_MAX;

    for (int i = 0; i < fs->options.diskbuf_count; i++) {
        if (fs->diskbuf[i] == NULL) {
            if (status < SHOULD_ALLOC) {
                target_entry_idx = i;
                status = SHOULD_ALLOC;
            }
        } else {
            if ((fs->diskbuf[i]->type == DISKBUF_TYPE_CLUSTER) &&
                (fs->diskbuf[i]->cluster == cluster)) {
                target_entry_idx = i;
                status = CACHED;
            } else if (
                (fs->diskbuf[i]->usage_tag < usage_tag_min) &&
                (status == SHOULD_REALLOC)) {
                usage_tag_min = fs->diskbuf[i]->usage_tag;
                target_entry_idx = i;
            }

            if (fs->diskbuf[i]->usage_tag > 0) {
                fs->diskbuf[i]->usage_tag--;
            }
        } 
    }

    if (status < CACHED) {
        if (status == SHOULD_REALLOC) {
            flush_diskbuf_entry(fs, target_entry_idx);
            if (fs->diskbuf[target_entry_idx]->type != DISKBUF_TYPE_CLUSTER) {
                fs->diskbuf[target_entry_idx] = realloc(
                    fs->diskbuf[target_entry_idx],
                    sizeof(struct diskbuf_entry) + fs->cluster_size);
            }
        } else {
            fs->diskbuf[target_entry_idx] = malloc(sizeof(struct diskbuf_entry) + fs->cluster_size);
        }
        fs->diskbuf[target_entry_idx]->type = DISKBUF_TYPE_CLUSTER;
        fs->diskbuf[target_entry_idx]->dirty = 0;
        fs->diskbuf[target_entry_idx]->data_valid = 0;
        fs->diskbuf[target_entry_idx]->cluster = cluster;
    }
    fs->diskbuf[target_entry_idx]->usage_tag = USAGE_TAG_MAX;

    *entry_idx = target_entry_idx;
    return 0;
#undef SHOULD_REALLOC
#undef SHOULD_ALLOC
#undef CACHED
}


/**
 * @brief Read a sector from disk and write into a diskbuf entry
 * 
 * @param fs filesystem object struct
 * @param entry_idx entry index output (NULL if not needed)
 * @param cluster cluster index
 * @return int 0 if success, otherwise failed
 */
static int read_cluster(struct fs_fat* fs, unsigned int* entry_idx, fatcluster_t cluster)
{
    unsigned int target_entry_idx;
    allocate_diskbuf_cluster_entry(fs, &target_entry_idx, cluster);

    lba_t lba;
    cluster_to_sector(fs, &lba, cluster);

    if (!fs->diskbuf[target_entry_idx]->data_valid) {
        ofsl_drive_read_sector(
            fs->part.drv,
            fs->diskbuf[target_entry_idx]->data,
            fs->part.lba_start + lba,
            fs->sector_size,
            fs->sectors_per_cluster);
        fs->diskbuf[target_entry_idx]->data_valid = 1;
    }

    if (entry_idx) {
        *entry_idx = target_entry_idx;
    }
    return 0;
}

/**
 * @brief Overwrite a cluster from the data of the given buffer.
 * 
 * @param fs filesystem object struct
 * @param entry_idx  entry index output (NULL if not needed)
 * @param buf cluster data buffer. The size of the buffer should equal or greater than the size of the cluster in bytes.
 * @param cluster cluster index
 * @return int int 0 if success, otherwise failed
 * 
 * @details
 * This function overwrites entire sector data with the data of the given buffer.
 * If you want to make changes in smaller units, follow these methods:
 * - Call read_cluster() with desired lba and get entry index.
 * - Make changes to the data of the entry and set the dirty bit. e.g.
 *   `fs->diskbuf[entry_idx]->dirty = 1`
 * - The changes will be written to the disk when the entry is flushed.
 */
static int write_cluster(struct fs_fat* fs, unsigned int* entry_idx, const void* buf, lba_t lba)
{
    unsigned int target_entry_idx;
    allocate_diskbuf_cluster_entry(fs, &target_entry_idx, lba);

    memcpy(fs->diskbuf[target_entry_idx]->data, buf, fs->cluster_size);
    fs->diskbuf[target_entry_idx]->dirty = 1;

    if (entry_idx) {
        *entry_idx = target_entry_idx;
    }
    return 0;
}

static int read_fat(struct fs_fat* fs, unsigned int* entry_idx, uint32_t sector_idx)
{
    return read_sector(fs, entry_idx, fs->reserved_sectors + sector_idx);
}

static int validate_sfn(const char* str, size_t len)
{
    /*  Characters Allowed:
        - A-Z
        - 0-9
        - char > 127
        - (space) $ % - _ @ ~ ` ! ( ) { } ^ # &

        Invalid Names:
        - .
        - ..

        Reference: https://averstak.tripod.com/fatdox/names.htm
     */
    static const uint32_t bitmap[] = {
        0x00000000, 0x03FF237B, /* ASCII 0x00 - 0x3F */
        0xC3FFFFFF, 0x68000001, /* ASCII 0x40 - 0x7F */
    };

    int has_dot = 0;

    if (len > 0 && str[0] == '.') {
        return 0;
    }
    
    for (int i = 0; str[i] != 0 && i < len; i++) {
        if (i > 0x7F) {
            continue;
        } else if (i == '.') {
            if (has_dot) {
                return 0;
            }
            has_dot = 1;
        }
        const uint32_t bmval = bitmap[str[i] >> 5];
        if (!((bmval >> (str[i] & 31)) & 1)) {
            return 0;
        }
    }
    return 1;
}

static int validate_lfn(const char* str, size_t len)
{
    /*  Characters Not Allowed:
        - \ / : * ? " < > |
        - Control characters

        Invalid Names:
        - .
        - ..

        Reference: https://en.wikipedia.org/wiki/Long_filename
     */
    static const uint32_t bitmap[] = {
        0x00000000, 0x23FF7BFB, /* ASCII 0x00 - 0x3F */
        0xFFFFFFFF, 0x6FFFFFFF, /* ASCII 0x40 - 0x7F */
    };

    if (len > 0 && str[0] == '.') {
        return 0;
    }
    
    for (int i = 0; str[i] != 0 && i < len; i++) {
        if (i > 0x7F) {
            continue;
        }
        const uint32_t bmval = bitmap[str[i] >> 5];
        if (!((bmval >> (str[i] & 31)) & 1)) {
            return 0;
        }
    }
    return 1;
}

static int ucs2_to_utf8(char* buf, int len, uint16_t ucs2ch)
{
    if (ucs2ch == 0 || ucs2ch == 0xFFFF) return 0;

    if (ucs2ch < 0x7F) {
        if (len < 1) return -1;
        *buf = ucs2ch;
        return 1;
    }

    if (ucs2ch < 0x7FF) {
        if (len < 2) return -1;
        *buf++ = ((ucs2ch & 0x07C0) >> 6) | 0xC0;
        *buf++ = (ucs2ch & 0x003F) | 0x80;
        return 2;
    }

    if (len < 3) return -1;
    *buf++ = ((ucs2ch & 0xF000) >> 12) | 0xE0;
    *buf++ = ((ucs2ch & 0x0FC0) >> 6) | 0x80;
    *buf++ = (ucs2ch & 0x003F) | 0x80;
    return 3;
}

static size_t get_lfn_filename(const struct fat_direntry_lfn* entry, uint16_t buf[static FAT_LFN_BUFLEN])
{
    buf += ((entry->sequence_index & 0x1F) - 1) * 13;
    size_t char_count = 0;
    for (int i = 0; i < 5; i++) {
        buf[char_count++] = entry->name_fragment1[i];
    }
    for (int i = 0; i < 6; i++) {
        buf[char_count++] = entry->name_fragment2[i];
    }
    for (int i = 0; i < 2; i++) {
        buf[char_count++] = entry->name_fragment3[i];
    }
    if (entry->sequence_index & 0x40) {
        buf[char_count] = '\0';
    }
    return char_count;
}

static size_t lfn_ucs2_to_utf8(char utf8buf[static FAT_LFN_U8_BUFLEN], const uint16_t ucs2buf[static FAT_LFN_BUFLEN], int allow_nonascii, char fallback)
{
    size_t bytes_written;
    if (allow_nonascii) {
        size_t buflen = FAT_LFN_U8_BUFLEN;
        for (bytes_written = 0; bytes_written < FAT_LFN_U8_BUFLEN; bytes_written++) {
            int u8ch_len = ucs2_to_utf8(utf8buf, buflen, *ucs2buf++);
            if (!u8ch_len) {
                *utf8buf = 0;
                break;
            } else if (u8ch_len < 0) {
                *utf8buf = fallback;
                u8ch_len = 1;
            }
            utf8buf += u8ch_len;
            bytes_written += u8ch_len;
            buflen -= u8ch_len;
        }
    } else {
        for (bytes_written = 0; bytes_written < FAT_LFN_BUFLEN; bytes_written++) {
            *utf8buf++ = *ucs2buf < 0x80 ? *ucs2buf : fallback;
            ucs2buf++;
        }
        *utf8buf = 0;
    }

    return bytes_written;
}

static size_t get_sfn_filename(const struct fat_direntry_file* entry, char buf[static FAT_SFN_BUFLEN], int lowercase)
{
    size_t char_count = 0;
    for (int i = 0; i < 8 && entry->name[i] != ' '; i++) {
        buf[char_count++] = lowercase ? tolower(entry->name[i]) : entry->name[i];
    }
    if (entry->extension[0] != ' ') {
        buf[char_count++] = '.';
    }
    for (int i = 0; i < 3 && entry->extension[i] != ' '; i++) {
        buf[char_count++] = lowercase ? tolower(entry->extension[i]) : entry->extension[i];
    }
    buf[char_count] = '\0';

    return char_count;
}

static uint8_t get_sfn_checksum(char buf[static FAT_SFN_BUFLEN])
{
    uint8_t chksum = 0;
    for (int i = FAT_SFN_BUFLEN; i != 0; i--) {
        chksum = ((chksum & 1) ? 0x80 : 0) + (chksum >> 1) + *buf++;
    }

    return chksum;
}

static const char* get_error_string(OFSL_FileSystem* fs_opaque)
{
    struct fs_fat* fs = (struct fs_fat*)fs_opaque;

    if (!fs) return NULL;
    if (fs->fs.error >= 0) return NULL;

    return error_str_list[-fs->fs.error - 1];
}

static int mount(OFSL_FileSystem* fs_opaque)
{
    struct fs_fat* fs = (struct fs_fat*)fs_opaque;

    fs->diskbuf = malloc(sizeof(struct diskbuf_entry*) * fs->options.diskbuf_count);

    fs->sector_size = 512;

    // Read sector 0 (BPB)
    unsigned int entry_idx;
    read_sector(fs, &entry_idx, 0);

    const struct fat_bpb_sector* bpb = (void*)fs->diskbuf[entry_idx]->data;

    fs->sector_size = bpb->bytes_per_sector;
    fs->sectors_per_cluster = bpb->sectors_per_cluster;
    fs->cluster_size = fs->sector_size * fs->sectors_per_cluster;
    fs->root_entry_count = bpb->root_entry_count;
    fs->reserved_sectors = bpb->reserved_sector_count;
    fs->root_sector_count = ((fs->root_entry_count * 32) + (fs->sector_size - 1)) / fs->sector_size;
    fs->fat_size = bpb->fat_size16 ? bpb->fat_size16 : bpb->fat32.fat_size32;
    fs->total_sector_count = bpb->total_sector_count16 ? bpb->total_sector_count16 : bpb->total_sector_count32;
    fs->data_area_begin = fs->reserved_sectors + (bpb->fat_count * fs->fat_size);
    uint32_t data_sectors = fs->total_sector_count - (fs->data_area_begin + fs->root_sector_count);
    uint32_t cluster_count = data_sectors / fs->sectors_per_cluster;

    // Determine FAT type
    if (cluster_count < 4085) {
        fs->fat_type = FAT_TYPE_FAT12;
    } else if (cluster_count < 65525) {
        fs->fat_type = FAT_TYPE_FAT16;
    } else {
        fs->fat_type = FAT_TYPE_FAT32;
    }

    // Read FSINFO if FAT32
    if (fs->fat_type == FAT_TYPE_FAT32) {
        fs->root_cluster = bpb->fat32.root_cluster;

        read_sector(fs, &entry_idx, 1);
        const struct fat_fsinfo* fsinfo = (void*)fs->diskbuf[entry_idx]->data;

        fs->free_clusters = fsinfo->free_clusters;
        fs->next_free_cluster = fsinfo->next_free_cluster;
    }

    fs->mounted = 1;

    return 0;
}

static int unmount(OFSL_FileSystem* fs_opaque)
{
    struct fs_fat* fs = check_fs_mounted(fs_opaque);
    if (!fs) return 1;

    for (int i = 0; i < fs->options.diskbuf_count; i++) {
        if (fs->diskbuf[i] != NULL) {
            free(fs->diskbuf[i]);
        }
    }
    free(fs->diskbuf);
    fs->mounted = 0;

    return 0;
}

static int get_mount_info(OFSL_FileSystem* fs_opaque, OFSL_MountInfo* mntinfo)
{
    struct fs_fat* fs = check_fs_mounted(fs_opaque);
    if (!fs) return 1;

    static const char* const fat_name[] = {
        "UNKNOWN",
        "FAT12",
        "FAT16",
        "FAT32",
    };

    static const size_t fat_max_part_sect_cnt[] = {
        0,

    };

    _ofsl_fs_mntinfo_init(mntinfo);
    mntinfo->fs_name = fat_name[fs->fat_type];
    mntinfo->vol_name = NULL;
    mntinfo->vol_serial = NULL;

    return 0;
}

static int dir_create(OFSL_Directory* dir, const char* name)
{
    return -1;
}

static int dir_remove(OFSL_Directory* dir, const char* name)
{
    return -1;
}

static OFSL_Directory* rootdir_open(OFSL_FileSystem* fs_opaque)
{
    struct fs_fat* fs = check_fs_mounted(fs_opaque);
    if (!fs) return NULL;

    struct dir_fat* dir = malloc(sizeof(struct dir_fat));
    dir->dir.ops = fs->fs.ops;
    dir->dir.fs = (OFSL_FileSystem*)fs;
    dir->child_count = 0;
    dir->parent = NULL;
    dir->head_cluster = fs->fat_type == FAT_TYPE_FAT32 ? fs->root_cluster : 0;

    return (OFSL_Directory*)dir;
}

static OFSL_Directory* dir_open(OFSL_Directory* parent_opaque, const char* name)
{
    struct dir_fat* parent = check_dir(parent_opaque);
    if (!parent) return NULL;
    struct fs_fat* fs = check_fs_mounted(parent->dir.fs);
    if (!fs) return NULL;

    struct fat_direntry_file dirent;
    if (!match_name((OFSL_Directory*)parent, name, &dirent)) {
        fs->fs.error = OFSL_FSE_NOENT;
        return NULL;
    }

    const uint16_t head_cluster_lo = dirent.cluster_location;
    const uint16_t head_cluster_hi = dirent.cluster_location_high;
    const uint32_t head_cluster = (head_cluster_hi << 16) | head_cluster_lo;

    struct dir_fat* dir = malloc(sizeof(struct dir_fat));
    dir->dir.ops = fs->fs.ops;
    dir->dir.fs = (OFSL_FileSystem*)fs;
    dir->head_cluster = head_cluster;
    dir->parent = parent;
    memcpy(&dir->direntry, &dirent, sizeof(dirent));

    parent->child_count++;

    return (OFSL_Directory*)dir;
}

static int dir_close(OFSL_Directory* dir_opaque)
{
    struct dir_fat* dir = check_dir(dir_opaque);
    if (!dir) return 1;
    struct dir_fat* parent = dir->parent;
    if (dir->child_count > 0) return 1;

    if (parent != NULL) {
        parent->child_count--;
    }

    free(dir);
    return 0;
}

static OFSL_DirectoryIterator* dir_iter_start(OFSL_Directory* dir_opaque)
{
    struct dir_fat* dir = check_dir(dir_opaque);
    if (!dir) return NULL;
    struct fs_fat* fs = check_fs_mounted(dir->dir.fs);
    if (!fs) return NULL;

    struct dirit_fat* it = malloc(sizeof(struct dirit_fat));
    it->dirit.fs = (OFSL_FileSystem*)fs;
    it->dirit.ops = fs->fs.ops;
    it->parent = dir;
    it->valid = 0;
    it->current_cluster_idx = 0;
    it->current_entry_idx = 0;

    return (OFSL_DirectoryIterator*)it;
}

static int dir_iter_next(OFSL_DirectoryIterator* it_opaque)
{
    struct dirit_fat* it = (struct dirit_fat*)it_opaque;
    if (!it) return 1;
    struct dir_fat* dir = it->parent;
    struct fs_fat* fs = check_fs_mounted(it->dirit.fs);
    if (!fs) return 1;

    uint16_t entry_count = 
        fs->fat_type != FAT_TYPE_FAT32 && dir->head_cluster == 0 ?
            fs->sector_size / sizeof(union fat_dir_entry) :
            fs->cluster_size / sizeof(union fat_dir_entry);
    uint16_t lfn_ucs2_buf[FAT_LFN_BUFLEN];
    int is_lfn = 0;

    unsigned int diskbuf_entry_idx;
    while (1) {
        if (fs->fat_type != FAT_TYPE_FAT32 && dir->head_cluster == 0) {
            /* root directory */
            read_sector(fs, &diskbuf_entry_idx, fs->data_area_begin + it->current_cluster_idx);
        } else {
            fatcluster_t current_cluster = dir->head_cluster;
            if (get_next_cluster(fs, &current_cluster, it->current_cluster_idx)) {
                free(it);
                return 1;
            }
            read_cluster(fs, &diskbuf_entry_idx, current_cluster);
        }
        union fat_dir_entry* entries = (union fat_dir_entry*)fs->diskbuf[diskbuf_entry_idx]->data;

        for (; it->current_entry_idx < entry_count; it->current_entry_idx++) {
            if (!entries[it->current_entry_idx].file.name[0]) {  // End of entry list
                return 1;
            } else if (entries[it->current_entry_idx].file.name[0] == 0xE5) {
                /* skip if file entry is deleted */
                continue;
            } else if ((entries[it->current_entry_idx].file.attribute & FAT_ATTR_LFNENTRY) == FAT_ATTR_LFNENTRY) {
                // if current entry is LFN entry
                if (fs->options.lfn_enabled) {
                    // write to buffer if LFN is enabled
                    get_lfn_filename(&entries[it->current_entry_idx].lfn, lfn_ucs2_buf);
                    is_lfn = 1;
                }  // otherwise skip
                continue;
            }

            if (is_lfn) {
                lfn_ucs2_to_utf8(it->filename, lfn_ucs2_buf, fs->options.unicode_enabled, fs->options.unknown_char_fallback);
            } else {
                get_sfn_filename(&entries[it->current_entry_idx].file, it->filename, fs->options.sfn_lowercase);
            }
            memcpy(&it->direntry, &entries[it->current_entry_idx], sizeof(struct fat_direntry_file));
            it->current_entry_idx++;
            return 0;
        }

        it->current_cluster_idx++;
        it->current_entry_idx = 0;
    }
}

static void dir_iter_end(OFSL_DirectoryIterator* it_opaque)
{
    free(it_opaque);
}

static const char* dir_iter_get_file_name(const OFSL_DirectoryIterator* it_opaque)
{
    const struct dirit_fat* it = (const struct dirit_fat*)it_opaque;
    return it->filename;
}

static int dirent_to_fattr(OFSL_FileAttribute* fattr, const struct fat_direntry_file* dirent)
{
    memset(fattr, 0, sizeof(*fattr));

    fattr->size       = dirent->size;
    fattr->directory  = (dirent->attribute & FAT_ATTR_DIRECTORY) == FAT_ATTR_DIRECTORY;
    fattr->immutable  = (dirent->attribute & FAT_ATTR_READ_ONLY) == FAT_ATTR_READ_ONLY;
    fattr->hidden     = (dirent->attribute & FAT_ATTR_HIDDEN) == FAT_ATTR_HIDDEN;
    fattr->system     = (dirent->attribute & FAT_ATTR_SYSTEM) == FAT_ATTR_SYSTEM;
    fattr->compressed = 0;
    fattr->encrypted  = 0;
    fattr->link       = 0;
    fattr->symlink    = 0;

    struct tm tm;
    tm.tm_sec  = dirent->created_time.second_div2 << 1;
    tm.tm_sec += dirent->created_tenth / 10;
    tm.tm_min  = dirent->created_time.minute;
    tm.tm_hour = dirent->created_time.hour;
    tm.tm_mday = dirent->created_date.day - 1;
    tm.tm_mon  = dirent->created_date.month - 1;
    tm.tm_year = dirent->created_date.year + 80;
    ofsl_time_fromstdctm(&fattr->time_created, &tm);

    tm.tm_sec  = dirent->modified_time.second_div2 << 1;
    tm.tm_min  = dirent->modified_time.minute;
    tm.tm_hour = dirent->modified_time.hour;
    tm.tm_mday = dirent->modified_date.day - 1;
    tm.tm_mon  = dirent->modified_date.month - 1;
    tm.tm_year = dirent->modified_date.year + 80;
    ofsl_time_fromstdctm(&fattr->time_modified, &tm);

    tm.tm_sec  = 0;
    tm.tm_min  = 0;
    tm.tm_hour = 0;
    tm.tm_mday = dirent->accessed_date.day - 1;
    tm.tm_mon  = dirent->accessed_date.month - 1;
    tm.tm_year = dirent->accessed_date.year + 80;
    ofsl_time_fromstdctm(&fattr->time_accessed, &tm);
    return 0;
}

static int dir_iter_get_attr(const OFSL_DirectoryIterator* it_opaque, OFSL_FileAttribute* fattr)
{
    const struct dirit_fat* it = (const struct dirit_fat*)it_opaque;

    return dirent_to_fattr(fattr, &it->direntry);
}

static int get_file_attr(OFSL_Directory* parent_opaque, const char* name, OFSL_FileAttribute* fattr)
{
    const struct dir_fat* parent = check_dir(parent_opaque);
    if (!parent) {
        return 1;
    }
    struct fs_fat* fs = check_fs_mounted(parent->dir.fs);
    if (!fs) {
        return 1;
    }

    struct fat_direntry_file dirent;
    if (!match_name((OFSL_Directory*)parent, name, &dirent)) {
        fs->fs.error = OFSL_FSE_NOENT;
        return 1;
    }

    if (dirent_to_fattr(fattr, &dirent)) {
        return 1;
    }
    
    return 0;
}

static int file_create(OFSL_Directory* path, const char* name)
{
    return -1;
}

static int file_remove(OFSL_Directory* path, const char* name)
{
    return -1;
}

static int match_name(OFSL_Directory* parent_opaque, const char* name, struct fat_direntry_file* direntry_buf)
{
    struct dir_fat* parent = check_dir(parent_opaque);
    if (!parent) {
        return 0;
    }
    struct fs_fat* fs = check_fs_mounted(parent->dir.fs);

    struct dirit_fat* it = (struct dirit_fat*)dir_iter_start(parent_opaque);
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

static OFSL_File* file_open(OFSL_Directory* parent_opaque, const char* name, const char* mode)
{
    struct dir_fat* parent = check_dir(parent_opaque);
    if (!parent) {
        return NULL;
    }
    struct fs_fat* fs = check_fs_mounted(parent->dir.fs);
    if (!fs) {
        return NULL;
    }

    struct fat_direntry_file dirent;
    if (!match_name((OFSL_Directory*)parent, name, &dirent)) {
        fs->fs.error = OFSL_FSE_NOENT;
        return NULL;
    }

    const uint16_t head_cluster_lo = dirent.cluster_location;
    const uint16_t head_cluster_hi = dirent.cluster_location_high;
    const uint32_t head_cluster = (head_cluster_hi << 16) | head_cluster_lo;

    struct file_fat* file = malloc(sizeof(struct file_fat));
    file->file.ops = fs->fs.ops;
    file->file.fs = (OFSL_FileSystem*)fs;
    file->head_cluster = head_cluster;
    file->parent = parent;
    file->cursor = 0;
    memcpy(&file->direntry, &dirent, sizeof(dirent));

    parent->child_count++;

    return (OFSL_File*)file;
}

static int file_close(OFSL_File* file_opaque)
{
    struct file_fat* file = check_file(file_opaque);
    if (!file) {
        return 1;
    }
    struct dir_fat* parent = file->parent;
    if (!parent) {
        return 1;
    }

    parent->child_count--;

    free(file);
    return 0;
}

static ssize_t file_read(OFSL_File* file_opaque, void* buf, size_t size, size_t count)
{
    struct file_fat* file = check_file(file_opaque);
    struct fs_fat* fs = check_fs_mounted(file->file.fs);
    struct fat_direntry_file* entry = &file->direntry;
    if (!file || !fs) {
        return -1;
    }

    if (file_iseof((OFSL_File*)file)) {
        return -1;
    }

    uint8_t* bbuf = buf;

    fatcluster_t cluster_idx = file->head_cluster;
    get_next_cluster(fs, &cluster_idx, file->cursor / fs->cluster_size);

    unsigned int entry_idx;

    for (size_t blkcnt = 0; blkcnt < count; blkcnt++) {
        if (file->cursor + size > file->direntry.size) {
            return blkcnt;
        }
        uint32_t cluster_offs = file->cursor % fs->cluster_size;
        size_t block_read_bytes = 0;

        for (;;) {
            uint16_t cluster_max_read = fs->cluster_size - cluster_offs;
            uint16_t block_max_read = size - block_read_bytes;

            read_cluster(fs, &entry_idx, cluster_idx);

            if (cluster_max_read > block_max_read) {
                memcpy(bbuf, fs->diskbuf[entry_idx]->data + cluster_offs, block_max_read);
                block_read_bytes += block_max_read;
                bbuf += block_max_read;
                file->cursor += block_max_read;
                break;
            } else {
                memcpy(bbuf, fs->diskbuf[entry_idx]->data + cluster_offs, cluster_max_read);
                block_read_bytes += cluster_max_read;
                cluster_offs = 0;
                bbuf += cluster_max_read;
                file->cursor += cluster_max_read;

                get_next_cluster(fs, &cluster_idx, 1);
            }
        }
    }

    return count;
}

static ssize_t file_write(OFSL_File* file, const void* buf, size_t size, size_t count)
{
    return -1;
}

static int file_seek(OFSL_File* file_opaque, ssize_t offset, int origin)
{
    struct file_fat* file = check_file(file_opaque);
    if (!file) {
        return 1;
    }
    struct fat_direntry_file* entry = &file->direntry;

    switch (origin) {
        case SEEK_SET:
            if (offset > entry->size || offset < 0) {
                return -1;
            } else {
                file->cursor = offset;
            }
            break;
        case SEEK_CUR:
            if (offset + file->cursor > entry->size || offset + file->cursor < 0) {
                return -1;
            } else {
                file->cursor += offset;
            }
            break;
        case SEEK_END:
            if (offset > 0 || offset + file->cursor < 0) {
                return -1;
            } else {
                file->cursor = entry->size + offset;
            }
            break;
        default:
            return 1;
    }
    return 0;
}

static int file_flush(OFSL_File* file_opaque)
{
    return -1;
}

static ssize_t file_tell(OFSL_File* file_opaque)
{
    struct file_fat* file = check_file(file_opaque);
    if (!file) {
        return -1;
    }
    struct fat_direntry_file* entry = &file->direntry;

    if (file->cursor < 0 || file->cursor > entry->size) {
        return -1;
    } else {
        return file->cursor;
    }
}

static int file_iseof(OFSL_File* file_opaque)
{
    struct file_fat* file = check_file(file_opaque);
    if (!file) {
        return -1;
    }
    struct fat_direntry_file* entry = &file->direntry;
    return file->cursor >= entry->size;
}

static void _delete(OFSL_FileSystem* fs_opaque)
{
    struct fs_fat* fs = (struct fs_fat*)fs_opaque;

    free(fs);
}

OFSL_FileSystem* ofsl_fs_fat_create(OFSL_Partition* part)
{
    static const struct ofsl_fs_ops fsops = {
        .get_error_string = get_error_string,
        ._delete = _delete,
        .mount = mount,
        .unmount = unmount,
        .get_mount_info = get_mount_info,
        .dir_create = dir_create,
        .dir_remove = dir_remove,
        .rootdir_open = rootdir_open,
        .dir_open = dir_open,
        .dir_close = dir_close,
        .dir_iter_start = dir_iter_start,
        .dir_iter_next = dir_iter_next,
        .dir_iter_end = dir_iter_end,
        .dir_iter_get_file_name = dir_iter_get_file_name,
        .dir_iter_get_attr = dir_iter_get_attr,
        .get_file_attr = get_file_attr,
        .file_create = file_create,
        .file_remove = file_remove,
        .file_open = file_open,
        .file_close = file_close,
        .file_read = file_read,
        .file_write = file_write,
        .file_seek = file_seek,
        .file_tell = file_tell,
        .file_iseof = file_iseof,
    };

    if (part->drv->drvinfo.sector_size < 512) {
        return NULL;
    }

    struct fs_fat* fs = malloc(sizeof(struct fs_fat));
    
    fs->part = *part;
    fs->fs.ops = &fsops;

    /* default settings */
    fs->options.diskbuf_count = DEFAULT_DISKBUF_ENTRY_COUNT;
    fs->options.lfn_enabled = DEFAULT_LFN_ENABLED;
    fs->options.readonly = DEFAULT_READONLY;
    fs->options.unicode_enabled = DEFAULT_UNICODE_ENABLED;
    fs->options.unknown_char_fallback = DEFAULT_UNKNOWN_CHAR_FALLBACK;
    fs->options.use_fsinfo_nextfree = DEFAULT_USE_FSINFO_NEXTFREE;
    fs->options.case_sensitive = DEFAULT_CASE_SENSITIVE;
    fs->options.sfn_lowercase = DEFAULT_SFN_LOWERCASE;

    fs->mounted = 0;

    return (OFSL_FileSystem*)fs;
}

struct ofsl_fs_fat_option* ofsl_fs_fat_get_option(OFSL_FileSystem* fs_opaque)
{
    struct fs_fat* fs = (struct fs_fat*)fs_opaque;

    return fs->mounted ? NULL : &fs->options;
}
