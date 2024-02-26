#include <assert.h>
#include <stdlib.h>
#include <ctype.h>

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include <ofsl/fs/fs.h>

#include "md5.h"

enum treefile_line_type {
    TFLINE_INVALID,
    TFLINE_DIRINFO,
    TFLINE_FILEINFO,
    TFLINE_DIRSTART,
    TFLINE_DONOTEVAL,
};

struct treefile_line {
    char filename[384];
    char md5str[17];
    unsigned int size;
    OFSL_Time created;
    OFSL_Time modified;
    OFSL_Time accessed;
};

static enum treefile_line_type parse_treefile_line(const char* line, struct treefile_line *presult)
{
    switch (strnlen(line, 3)) {
        case 1:
            if (line[0] != '\n')
                return TFLINE_INVALID;
        case 0:
            return TFLINE_DONOTEVAL;
    }

    switch (line[0]) {
        case '#':
            return TFLINE_DONOTEVAL;
        case '$':
            if (sscanf(line, "$ %383s\n", &presult->filename) == 1) {
                return TFLINE_DIRSTART;
            } else {
                return TFLINE_INVALID;
            }
        default:
            if (sscanf(
                line,
                "%383s %u %u-%u-%u %u:%u %s\n",
                presult->filename,
                &presult->size,
                &presult->year,
                &presult->month,
                &presult->day,
                &presult->hour,
                &presult->minute,
                presult->md5str) == 8) {
                return TFLINE_FILEINFO;
            } else if (sscanf(line, "%383s dir\n", presult->filename) == 1) {
                return TFLINE_DIRINFO;
            } else {
                return TFLINE_INVALID;
            }
    }
}

void testroutine_dir_list(OFSL_FileSystem* fs, const char* imgtreefile_path)
{
    OFSL_Directory* rootdir = ofsl_fs_rootdir_open(fs);
    OFSL_Directory* subdir = NULL;

    FILE* imgtree = fopen(imgtreefile_path, "r");

    OFSL_FileInfo* finfo = ofsl_dir_list_start(rootdir);

    char line[512];
    struct treefile_line presult;

    while (fgets(line, sizeof(line), imgtree)) {
        enum treefile_line_type line_type = parse_treefile_line(line, &presult);
        switch (line_type) {
            case TFLINE_INVALID:
            case TFLINE_DIRINFO:
                CU_ASSERT_FALSE(ofsl_dir_list_next(finfo));
                CU_ASSERT_STRING_EQUAL(presult.filename, ofsl_get_file_name(finfo));
                break;
            case TFLINE_FILEINFO:
                CU_ASSERT_FALSE(ofsl_dir_list_next(finfo));
                CU_ASSERT_EQUAL(ofsl_get_file_size(finfo), fsize);
                OFSL_Time created_time = ofsl_get_file_created_time(finfo);
                CU_ASSERT_EQUAL(created_time.year, presult.year);
                CU_ASSERT_EQUAL(created_time.month, presult.month);
                CU_ASSERT_EQUAL(created_time.day, presult.day);
                CU_ASSERT_EQUAL(created_time.hour, presult.hour);
                CU_ASSERT_EQUAL(created_time.minute, presult.minute);
                CU_ASSERT_EQUAL(created_time.second, presult.second);
                CU_ASSERT_EQUAL(created_time.nsec10, presult.nsec10);
                OFSL_Time accessed_time = ofsl_get_file_accessed_time(finfo);
                CU_ASSERT_EQUAL(accessed_time.year, presult.year);
                CU_ASSERT_EQUAL(accessed_time.month, presult.month);
                CU_ASSERT_EQUAL(accessed_time.day, presult.day);
                CU_ASSERT_EQUAL(accessed_time.hour, presult.hour);
                CU_ASSERT_EQUAL(accessed_time.minute, presult.minute);
                CU_ASSERT_EQUAL(accessed_time.nsec10, presult.nsec10);
                OFSL_Time modified_time = ofsl_get_file_modified_time(finfo);
                CU_ASSERT_EQUAL(year, modified_time.year);
                CU_ASSERT_EQUAL(month, modified_time.month);
                CU_ASSERT_EQUAL(day, modified_time.day);
                CU_ASSERT_EQUAL(0,  modified_time.hour);
                CU_ASSERT_EQUAL(0, modified_time.minute);
                CU_ASSERT_STRING_EQUAL(fname_buf, ofsl_get_file_name(finfo));
                break;
            case TFLINE_DIRSTART:
                CU_ASSERT_TRUE_FATAL(ofsl_dir_list_next(finfo));
                ofsl_dir_list_end(finfo);
                if (subdir) {
                    ofsl_dir_close(subdir);
                }
                subdir = ofsl_dir_open(rootdir, fname_buf);
                CU_ASSERT_PTR_NOT_NULL_FATAL(subdir);
                finfo = ofsl_dir_list_start(subdir);
                CU_ASSERT_PTR_NOT_NULL_FATAL(finfo);
            case TFLINE_DONOTEVAL:
                break;
        }
    }
    ofsl_dir_list_end(finfo);

    if (subdir) {
        ofsl_dir_close(subdir);
    }

    fclose(imgtree);

    ofsl_dir_close(rootdir);
}

void testroutine_file_read(OFSL_FileSystem* fs, const char* imgtreefile_path)
{
    OFSL_Directory* rootdir = ofsl_fs_rootdir_open(fs);
    OFSL_Directory* subdir = NULL;

    FILE* imgtree = fopen(imgtreefile_path, "r");

    char line_buf[512];
    char fname_buf[384];
    char md5str_buf[2][33];
    unsigned int fsize;
    
    while (fgets(line_buf, sizeof(line_buf), imgtree)) {
        if (strnlen(line_buf, 3) > 1) {
            switch (line_buf[0]) {
                case '#':
                    break;
                case '$':
                    if (sscanf(line_buf, "$ %383s\n", fname_buf) == 1) {
                        if (subdir) {
                            ofsl_dir_close(subdir);
                        }
                        subdir = ofsl_dir_open(rootdir, fname_buf);
                        CU_ASSERT_PTR_NOT_NULL_FATAL(subdir);
                    }
                    break;
                default:
                    if (sscanf(line_buf, "%383s %u %*u-%*u-%*u %*u:%*u %s\n", fname_buf, &fsize, md5str_buf[0]) == 3) {
                        /* file */
                        uint8_t* file_data = malloc(fsize);
                        OFSL_File* file = ofsl_file_open(subdir ? subdir : rootdir, fname_buf, "r");
                        CU_ASSERT_PTR_NOT_NULL_FATAL(file);

                        CU_ASSERT_EQUAL(ofsl_file_read(file, file_data, fsize, 1), 1);

                        /* check with MD5 algorithm */
                        MD5Context md5ctx;
                        md5Init(&md5ctx);
                        md5Update(&md5ctx, file_data, fsize);
                        md5Finalize(&md5ctx);

                        int correct_bytes = 0;
                        for (int i = 0; i < 16; i++) {
                            uint8_t upper = (md5ctx.digest[i] & 0xF0) >> 4;
                            uint8_t lower = md5ctx.digest[i] & 0x0F;
                            md5str_buf[1][i << 1] = upper < 10 ? upper + '0' : upper + 'a' - 10;
                            md5str_buf[1][(i << 1) + 1] = lower < 10 ? lower + '0' : lower + 'a' - 10;
                        }
                        md5str_buf[0][32] = 0;
                        md5str_buf[1][32] = 0;
                        CU_ASSERT_STRING_EQUAL(md5str_buf[0], md5str_buf[1]);

                        CU_ASSERT_FALSE(ofsl_file_seek(file, 0, SEEK_SET));
                        CU_ASSERT_EQUAL(ofsl_file_tell(file), 0);

                        CU_ASSERT_FALSE(ofsl_file_seek(file, 10, SEEK_CUR));
                        CU_ASSERT_EQUAL(ofsl_file_tell(file), 10);

                        CU_ASSERT_FALSE(ofsl_file_seek(file, 0, SEEK_END));
                        CU_ASSERT_EQUAL(ofsl_file_tell(file), fsize);
                        CU_ASSERT_TRUE(ofsl_file_iseof(file));

                        /* invalid seeks */
                        CU_ASSERT_TRUE(ofsl_file_seek(file, 1, SEEK_CUR));
                        CU_ASSERT_TRUE(ofsl_file_seek(file, -2048, SEEK_CUR));
                        CU_ASSERT_TRUE(ofsl_file_seek(file, 1, SEEK_END));
                        CU_ASSERT_TRUE(ofsl_file_seek(file, -2048, SEEK_END));
                        CU_ASSERT_TRUE(ofsl_file_seek(file, -1, SEEK_SET));
                        CU_ASSERT_TRUE(ofsl_file_seek(file, 2048, SEEK_SET));
                        CU_ASSERT_TRUE(ofsl_file_seek(file, 0, 4));

                        /* read file in invalid location */
                        CU_ASSERT_EQUAL(ofsl_file_read(file, file_data, 1, 1), -1);

                        /* partial success */
                        CU_ASSERT_FALSE(ofsl_file_seek(file, -1, SEEK_CUR));
                        CU_ASSERT_EQUAL(ofsl_file_read(file, file_data, 1, 2), 1);

                        ofsl_file_close(file);
                        free(file_data);
                    }
                    break;
            }
        }
    }

    if (subdir) {
        ofsl_dir_close(subdir);
    }

    fclose(imgtree);

    ofsl_dir_close(rootdir);
}
