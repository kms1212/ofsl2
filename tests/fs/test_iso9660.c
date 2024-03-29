#include <assert.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include <ofsl/drive/rawimage.h>
#include <ofsl/fs/iso9660.h>
#include <ofsl/partition/partition.h>

#include "md5.h"

#define TEST_SECTOR_SIZE 2048

OFSL_Drive* drive;
OFSL_FileSystem* isofs;
const char* fsname_expected;
const char* imgtree_path;

static int init_test_suite(void)
{
    drive = ofsl_drive_rawimage_create("tests/data/iso9660/image.iso", 0, TEST_SECTOR_SIZE);
    assert(drive);

    OFSL_Partition part;
    ofsl_partition_from_drive(&part, drive);

    isofs = ofsl_fs_iso9660_create(&part);
    assert(isofs);

    fsname_expected = "ISO9660";
    imgtree_path = "tests/data/iso9660/image-tree.txt";
    return 0;
}

static void test_mount(void)
{
    CU_ASSERT_FALSE(ofsl_fs_mount(isofs));
}

static void test_vol_str(void)
{
    char str_buf[129];
    ofsl_fs_get_volume_string(isofs, OFSL_VSTYPE_LABEL, str_buf, sizeof(str_buf));
    CU_ASSERT_STRING_EQUAL(str_buf, "ISO9660FS TEST");
    ofsl_fs_get_volume_string(isofs, OFSL_VSTYPE_PUBLISHER, str_buf, sizeof(str_buf));
    CU_ASSERT_STRING_EQUAL(str_buf, "ISO9660FS TEST");
    ofsl_fs_get_volume_string(isofs, OFSL_VSTYPE_AUTHOR, str_buf, sizeof(str_buf));
    CU_ASSERT_STRING_EQUAL(str_buf, "ISO9660FS TEST");
    ofsl_fs_get_volume_string(isofs, OFSL_VSTYPE_PROGRAM, str_buf, sizeof(str_buf));
    CU_ASSERT_STRING_EQUAL(str_buf, "ISO9660FS TEST");
    ofsl_fs_get_volume_string(isofs, OFSL_VSTYPE_COPYRIGHT, str_buf, sizeof(str_buf));
    CU_ASSERT_STRING_EQUAL(str_buf, "SOME_FILE");
    ofsl_fs_get_volume_string(isofs, OFSL_VSTYPE_ABSTRACT, str_buf, sizeof(str_buf));
    CU_ASSERT_STRING_EQUAL(str_buf, "SOME_FILE");
    ofsl_fs_get_volume_string(isofs, OFSL_VSTYPE_BIBLIOGRAPHY, str_buf, sizeof(str_buf));
    CU_ASSERT_STRING_EQUAL(str_buf, "SOME_FILE");
}

static void test_dir_list(void)
{
    OFSL_Directory* rootdir = ofsl_fs_rootdir_open(isofs);
    OFSL_Directory* subdir = NULL;

    FILE* imgtree = fopen(imgtree_path, "r");

    OFSL_DirectoryIterator* it = ofsl_dir_iter_start(rootdir);

    char line_buf[512];
    char fname_buf[384];
    unsigned int fsize, year, month, day, hour, minute, second;

    while (fgets(line_buf, sizeof(line_buf), imgtree)) {
        if (strnlen(line_buf, 3) > 1) {
            switch (line_buf[0]) {
                case '#':
                    break;
                case '$':
                    if (sscanf(line_buf, "$ %383s\n", fname_buf) == 1) {
                        CU_ASSERT_TRUE_FATAL(ofsl_dir_iter_next(it));
                        ofsl_dir_iter_end(it);
                        if (subdir) {
                            ofsl_dir_close(subdir);
                        }
                        subdir = ofsl_dir_open(rootdir, fname_buf);
                        CU_ASSERT_PTR_NOT_NULL_FATAL(subdir);
                        it = ofsl_dir_iter_start(subdir);
                        CU_ASSERT_PTR_NOT_NULL_FATAL(it);
                    }
                    break;
                default: {
                    if (sscanf(
                        line_buf,
                        "%383s %u %u-%u-%u %u:%u:%u %*s\n",
                        fname_buf, &fsize,  &year, &month, &day, &hour, &minute, &second) == 8) {
                        /* file */
                        CU_ASSERT_FALSE_FATAL(ofsl_dir_iter_next(it));

                        CU_ASSERT_EQUAL(ofsl_dir_iter_get_type(it), OFSL_FTYPE_FILE);

                        size_t size;
                        CU_ASSERT_FALSE_FATAL(ofsl_dir_iter_get_size(it, &size));
                        CU_ASSERT_EQUAL(size, fsize);
            
                        struct tm stdctime;
                        OFSL_Time ofsltime;
                        CU_ASSERT_FALSE_FATAL(ofsl_dir_iter_get_timestamp(it, OFSL_TSTYPE_CREATION, &ofsltime));
                        ofsl_time_getstdctm(&stdctime, &ofsltime);
                        CU_ASSERT_EQUAL(stdctime.tm_year, year - 1900);
                        CU_ASSERT_EQUAL(stdctime.tm_mon, month - 1);
                        CU_ASSERT_EQUAL(stdctime.tm_mday, day);
                        CU_ASSERT_EQUAL(stdctime.tm_hour, hour);
                        CU_ASSERT_EQUAL(stdctime.tm_min, minute);
                        CU_ASSERT_EQUAL(stdctime.tm_sec, second);
                    } else if (sscanf(line_buf, "%383s dir\n", fname_buf) == 1) {
                        /* directory */
                        CU_ASSERT_FALSE(ofsl_dir_iter_next(it));
                    } else {
                        /* unrecognized */
                        break;
                    }
                    CU_ASSERT_STRING_EQUAL(fname_buf, ofsl_dir_iter_get_name(it));
                    break;
                }
            }
        }
    }
    ofsl_dir_iter_end(it);

    if (subdir) {
        ofsl_dir_close(subdir);
    }

    fclose(imgtree);

    ofsl_dir_close(rootdir);
}

static void test_file_read(void)
{
    OFSL_Directory* rootdir = ofsl_fs_rootdir_open(isofs);
    OFSL_Directory* subdir = NULL;

    FILE* imgtree = fopen(imgtree_path, "r");

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
                    if (sscanf(
                        line_buf,
                        "%383s %u %*u-%*u-%*u %*u:%*u:%*u %s\n",
                        fname_buf, &fsize, md5str_buf[0]) == 3) {
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
                        printf("%s, %s\n", md5str_buf[0], md5str_buf[1]);

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

static void test_unmount(void)
{
    CU_ASSERT_FALSE(ofsl_fs_unmount(isofs));
}

static int clean_test_suite(void)
{
    ofsl_fs_delete(isofs);
    ofsl_drive_delete(drive);
    return 0;
}

int main(int argc, char** argv)
{
    CU_pSuite pSuite = NULL;

    if (CU_initialize_registry() != CUE_SUCCESS) {
        return CU_get_error();
    }

    static CU_TestInfo tests[] = {
        {
            .pName = "mount",
            .pTestFunc = test_mount
        },
        {
            .pName = "volume string",
            .pTestFunc = test_vol_str
        },
        {
            .pName = "directory list",
            .pTestFunc = test_dir_list
        },
        {
            .pName = "file read",
            .pTestFunc = test_file_read,
        },
        {
            .pName = "unmount",
            .pTestFunc = test_unmount
        },
        CU_TEST_INFO_NULL
    };

    static CU_SuiteInfo suites[] = {
        {
            .pName = "fs/iso9660",
            .pInitFunc = init_test_suite,
            .pCleanupFunc = clean_test_suite,
            .pTests = tests
        },
        CU_SUITE_INFO_NULL
    };

    if (CU_register_suites(suites)) {
        goto error_exit;
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();

    int tests_failed = CU_get_run_summary()->nTestsFailed;
    CU_cleanup_registry();
    return tests_failed;

error_exit:
    CU_cleanup_registry();
    return CU_get_error();
}
