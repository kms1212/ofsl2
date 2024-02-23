#include <ofsl/fs/fs.h>

#include <ofsl/fs/errmsg.h>

static const char* error_str_list[] = {
    "Operation successfully finished",
    "No such file or directory",
    "Unmounted file system",
    "Invalid cluster index",
    "Invalid file system type",
    "Invalid file or directory name",
};

const char* ofsl_fs_get_error_string(OFSL_FileSystem* fs)
{
    if (fs->error >= 0) {
        if (fs->error > OFSL_FSE_MAX) {
            return error_str_list[fs->error];
        } else {
            return NULL;
        }
    } else {
        return fs->ops->get_error_string(fs);
    }
}
