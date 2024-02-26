#include <ofsl/fs/iso9660.h>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <ofsl/drive/drive.h>
#include <ofsl/time.h>

#include "src/endian.h"
#include "fs/internal.h"
#include "fs/iso9660/internal.h"

struct fs_iso {
    OFSL_FileSystem fs;
    OFSL_Partition part;
    unsigned int vol_set_idx;
    uint8_t mounted : 1;
};

static struct fs_iso* check_fs_mounted(OFSL_FileSystem* fs_opaque)
{
    struct fs_iso* fs = (struct fs_iso*)fs_opaque;
    if (fs == NULL) {
        return NULL;
    } else if (!fs->mounted) {
        fs->fs.error = OFSL_FSE_UNMOUNTED;
        return NULL;
    }
    return fs;
}


static int mount(OFSL_FileSystem* fs_opaque)
{
    struct fs_iso* fs = (struct fs_iso*)fs_opaque;

    uint8_t sector_buf[2048];
    lba_t lba_primary_descriptor = 0;
    lba_t lba_current_descriptor = 16;
    struct isofs_vol_desc* voldesc = (struct isofs_vol_desc*)sector_buf;
    unsigned int current_vol_set = 0;

    for (unsigned int current_vol_set = 0; current_vol_set <= fs->vol_set_idx; current_vol_set++) {
        lba_primary_descriptor = 0;
        do {
            ofsl_drive_read_sector(fs->part.drv, sector_buf, lba_current_descriptor, sizeof(sector_buf), 1);
            switch (voldesc->type) {
                case VDTYPE_PRIVOLDESC:
                    if (!lba_primary_descriptor) {
                        lba_primary_descriptor = lba_current_descriptor;
                    }
                    break;
            }
            lba_current_descriptor++;
        } while (voldesc->type != VDTYPE_VDSETTERM);
    }

    if (!lba_primary_descriptor) {
        return 1;
    }

    ofsl_drive_read_sector(fs->part.drv, sector_buf, lba_primary_descriptor, sizeof(sector_buf), 1);

    if (strncmp(voldesc->signature, "CD001", 5) != 0) {
        return 1;
    }

    fs->mounted = 1;

    return 0;
}

static int unmount(OFSL_FileSystem* fs_opaque)
{
    struct fs_iso* fs = check_fs_mounted(fs_opaque);
    if (!fs) {
        return 1;
    }

    fs->mounted = 0;
    return 0;
}

static void _delete(OFSL_FileSystem* fs_opaque)
{
    struct fs_iso* fs = (struct fs_iso*)fs_opaque;

    free(fs);
}

OFSL_FileSystem* ofsl_fs_iso9660_create(OFSL_Partition* part, unsigned int vol_set_idx)
{
    static const struct ofsl_fs_ops fsops = {
        // .get_error_string = get_error_string,
        ._delete = _delete,
        .mount = mount,
        .unmount = unmount,
        // .get_mount_info = get_mount_info,
        // .dir_create = dir_create,
        // .dir_remove = dir_remove,
        // .rootdir_open = rootdir_open,
        // .dir_open = dir_open,
        // .dir_close = dir_close,
        // .dir_list_start = dir_list_start,
        // .dir_list_next = dir_list_next,
        // .dir_list_end = dir_list_end,
        // .get_file_info = get_file_info,
        // .get_file_name = get_file_name,
        // .get_file_attrib = get_file_attrib,
        // .get_file_created_time = get_file_created_time,
        // .get_file_modified_time = get_file_modified_time,
        // .get_file_accessed_time = get_file_accessed_time,
        // .get_file_size = get_file_size,
        // .file_create = file_create,
        // .file_remove = file_remove,
        // .file_open = file_open,
        // .file_close = file_close,
        // .file_read = file_read,
        // .file_write = file_write,
        // .file_seek = file_seek,
        // .file_flush = file_flush,
        // .file_tell = file_tell,
        // .file_iseof = file_iseof,
    };

    if (part->drv->drvinfo.sector_size < 2048) {
        return NULL;
    }

    struct fs_iso* fs = malloc(sizeof(struct fs_iso));
    
    fs->fs.ops = &fsops;
    fs->part = *part;
    fs->vol_set_idx = vol_set_idx;

    fs->mounted = 0;

    return (OFSL_FileSystem*)fs;
}
