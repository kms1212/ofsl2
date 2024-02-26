/* https://github.com/Zunawe/md5-c/blob/main/md5.h */

#ifndef ROUTINES_H
#define ROUTINES_H

#include <ofsl/fs/fs.h>

void testroutine_dir_list(OFSL_FileSystem* fs, const char* imgtreefile_path);
void testroutine_file_read(OFSL_FileSystem* fs, const char* imgtreefile_path);

#endif
