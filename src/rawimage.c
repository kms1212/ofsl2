#include <ofsl/drive/rawimage.h>

#include <stdio.h>
#include <stdlib.h>

struct drive_rawimage {
    OFSL_Drive drv;
    FILE* fp;
};

static int update_info(OFSL_Drive* drv_opaque)
{
    return 0;
}

static ssize_t read_sector(OFSL_Drive* drv_opaque, void* buf, lba_t lba, size_t sector_size, size_t cnt)
{
    struct drive_rawimage* drv = (struct drive_rawimage*)drv_opaque;
    const uint16_t img_sector_size = drv->drv.drvinfo.bytes_per_sector;

    if (sector_size > img_sector_size) {
        return 0;
    }

    uint8_t* bbuf = buf;
    for (size_t i = 0; i < cnt; i++) {
        fseek(drv->fp, lba * img_sector_size, SEEK_SET);

        if (!fread(bbuf, sector_size, 1, drv->fp)) {
            return i;
        } else {
            bbuf += sector_size;
            lba++;
        }
    }

    return cnt;
}

static ssize_t write_sector(OFSL_Drive* drv_opaque, const void* buf, lba_t lba, size_t sector_size, size_t cnt)
{
    struct drive_rawimage* drv = (struct drive_rawimage*)drv_opaque;
    const uint16_t img_sector_size = drv->drv.drvinfo.bytes_per_sector;

    if (sector_size > img_sector_size) {
        return 0;
    }

    const uint8_t* bbuf = buf;
    for (size_t i = 0; i < cnt; i++) {
        fseek(drv->fp, lba * img_sector_size, SEEK_SET);

        if (!fwrite(bbuf, sector_size, 1, drv->fp)) {
            return i;
        } else {
            bbuf += sector_size;
            lba++;
        }
    }

    return cnt;
}

static void _delete(OFSL_Drive* drv_opaque)
{
    struct drive_rawimage* drv = (struct drive_rawimage*)drv_opaque;

    fclose(drv->fp);
    free(drv);
}

OFSL_Drive* ofsl_create_rawimage_drive(const char* name, int readonly, size_t sector_size, size_t sector_count)
{
    static const struct ofsl_drive_ops drvops = {
        ._delete = _delete,
        .update_info = update_info,
        .read_sector = read_sector,
        .write_sector = write_sector,
    };

    FILE* fp = fopen(name, readonly ? "rb" : "rb+");
    if (fp == NULL) {
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    size_t filesz = ftell(fp);

    if (filesz != sector_size * sector_count) {
        fclose(fp);
        return NULL;
    }

    struct drive_rawimage* drv = malloc(sizeof(struct drive_rawimage));
    drv->drv.ops = &drvops;
    drv->drv.drvinfo.bytes_per_sector = sector_size;
    drv->drv.drvinfo.sector_count = sector_count;
    drv->drv.drvinfo.readonly = readonly;
    drv->fp = fp;

    return (OFSL_Drive*)drv;
}
