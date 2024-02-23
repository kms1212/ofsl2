#include <assert.h>
#include <stdlib.h>
#include <ctype.h>

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include <ofsl/drive/rawimage.h>
#include <ofsl/fs/fat.h>

#include "md5.h"

#define TEST_SECTOR_SIZE 512

OFSL_Drive* drive;
OFSL_FileSystem* fat;
const char* fsname_expected;
const char* imgtree_path;

static int init_fat12_suite(void)
{
    drive = ofsl_create_rawimage_drive("tests/data/fat/fat12.img", 0, TEST_SECTOR_SIZE, 2048);
    assert(drive);

    fat = ofsl_create_fs_fat(drive, 0);
    assert(fat);

    fsname_expected = "FAT12";
    imgtree_path = "tests/data/fat/fat12-tree.txt";
    return 0;
}

static int init_fat12_cl_suite(void)
{
    drive = ofsl_create_rawimage_drive("tests/data/fat/fat12.img", 0, TEST_SECTOR_SIZE, 2048);
    assert(drive);

    fat = ofsl_create_fs_fat(drive, 0);
    assert(fat);

    struct ofsl_fs_fat_option* options = ofsl_fs_fat_get_option(fat);
    options->case_sensitive = 1;
    options->sfn_lowercase = 1;
    options->unicode_enabled = 0;

    fsname_expected = "FAT12";
    imgtree_path = "tests/data/fat/fat12cl-tree.txt";
    return 0;
}

static int init_fat12_clu_suite(void)
{
    drive = ofsl_create_rawimage_drive("tests/data/fat/fat12.img", 0, TEST_SECTOR_SIZE, 2048);
    assert(drive);

    fat = ofsl_create_fs_fat(drive, 0);
    assert(fat);

    struct ofsl_fs_fat_option* options = ofsl_fs_fat_get_option(fat);
    options->case_sensitive = 1;
    options->sfn_lowercase = 1;

    fsname_expected = "FAT12";
    imgtree_path = "tests/data/fat/fat12clu-tree.txt";
    return 0;
}

static int init_fat12_csl_suite(void)
{
    drive = ofsl_create_rawimage_drive("tests/data/fat/fat12.img", 0, TEST_SECTOR_SIZE, 2048);
    assert(drive);

    fat = ofsl_create_fs_fat(drive, 0);
    assert(fat);

    struct ofsl_fs_fat_option* options = ofsl_fs_fat_get_option(fat);
    options->case_sensitive = 1;
    options->lfn_enabled = 0;
    options->sfn_lowercase = 1;

    fsname_expected = "FAT12";
    imgtree_path = "tests/data/fat/fat12csl-tree.txt";
    return 0;
}

static int init_fat16_suite(void)
{
    drive = ofsl_create_rawimage_drive("tests/data/fat/fat16.img", 0, TEST_SECTOR_SIZE, 32768);
    assert(drive);

    fat = ofsl_create_fs_fat(drive, 0);
    assert(fat);

    fsname_expected = "FAT16";
    imgtree_path = "tests/data/fat/fat16-tree.txt";
    return 0;
}

static int init_fat32_suite(void)
{
    drive = ofsl_create_rawimage_drive("tests/data/fat/fat32.img", 0, TEST_SECTOR_SIZE, 131072);
    assert(drive);

    fat = ofsl_create_fs_fat(drive, 0);
    assert(fat);

    fsname_expected = "FAT32";
    imgtree_path = "tests/data/fat/fat32-tree.txt";
    return 0;
}

static void test_mount(void)
{
    CU_ASSERT_FALSE(ofsl_fs_mount(fat));
}

static void test_unmount(void)
{
    CU_ASSERT_FALSE(ofsl_fs_unmount(fat));
}

static void test_get_filesystem_name(void)
{
    CU_ASSERT_STRING_EQUAL(ofsl_fs_get_filesystem_name(fat), fsname_expected);
}

static void test_no_such_entry(void)
{
    OFSL_Directory* rootdir = ofsl_fs_rootdir_open(fat);

    CU_ASSERT_PTR_NULL(ofsl_dir_open(rootdir, "unavailable.directory"));
    CU_ASSERT_PTR_NULL(ofsl_file_open(rootdir, "unavailable.file", "r"));

    ofsl_dir_close(rootdir);
}

static void test_dir_list(void)
{
    OFSL_Directory* rootdir = ofsl_fs_rootdir_open(fat);
    OFSL_Directory* subdir = NULL;

    FILE* imgtree = fopen(imgtree_path, "r");

    OFSL_FileInfo* finfo = ofsl_dir_list_start(rootdir);

    char line_buf[512];
    char fname_buf[384];
    unsigned int fsize, year, month, day, hour, minute;
    
    while (fgets(line_buf, sizeof(line_buf), imgtree)) {
        if (strnlen(line_buf, 3) > 1) {
            switch (line_buf[0]) {
                case '#':
                    break;
                case '$': {
                    if (sscanf(line_buf, "$ %383s\n", fname_buf) == 1) {
                        CU_ASSERT_TRUE_FATAL(ofsl_dir_list_next(finfo));
                        ofsl_dir_list_end(finfo);
                        if (subdir) {
                            ofsl_dir_close(subdir);
                        }
                        subdir = ofsl_dir_open(rootdir, fname_buf);
                        CU_ASSERT_PTR_NOT_NULL_FATAL(subdir);
                        finfo = ofsl_dir_list_start(subdir);
                        CU_ASSERT_PTR_NOT_NULL_FATAL(finfo);
                    }
                    break;
                }
                default: {
                    if (sscanf(line_buf, "%383s %u %u-%u-%u %u:%u %*s\n", fname_buf, &fsize, &year, &month, &day, &hour, &minute) == 7) {
                        /* file */
                        CU_ASSERT_FALSE(ofsl_dir_list_next(finfo));
                        CU_ASSERT_EQUAL(ofsl_get_file_size(finfo), fsize);
                        ofsl_time_t created_time = ofsl_get_file_created_time(finfo);
                        CU_ASSERT_EQUAL(year, created_time.year);
                        CU_ASSERT_EQUAL(month, created_time.month);
                        CU_ASSERT_EQUAL(day, created_time.day);
                        CU_ASSERT_EQUAL(hour, created_time.hour);
                        CU_ASSERT_EQUAL(minute, created_time.minute);
                        ofsl_time_t accessed_time = ofsl_get_file_accessed_time(finfo);
                        CU_ASSERT_EQUAL(year, accessed_time.year);
                        CU_ASSERT_EQUAL(month, accessed_time.month);
                        CU_ASSERT_EQUAL(day, accessed_time.day);
                        CU_ASSERT_EQUAL(0,  accessed_time.hour);
                        CU_ASSERT_EQUAL(0, accessed_time.minute);
                        ofsl_time_t modified_time = ofsl_get_file_modified_time(finfo);
                        CU_ASSERT_EQUAL(year, modified_time.year);
                        CU_ASSERT_EQUAL(month, modified_time.month);
                        CU_ASSERT_EQUAL(day, modified_time.day);
                        CU_ASSERT_EQUAL(0,  modified_time.hour);
                        CU_ASSERT_EQUAL(0, modified_time.minute);
                    } else if (sscanf(line_buf, "%383s dir\n", fname_buf) == 1) {
                        /* directory */
                        CU_ASSERT_FALSE(ofsl_dir_list_next(finfo));
                    } else {
                        /* unrecognized */
                        break;
                    }
                    printf("%s, %s\n", fname_buf, ofsl_get_file_name(finfo));
                    CU_ASSERT_STRING_EQUAL(fname_buf, ofsl_get_file_name(finfo));
                    break;
                }
            }
        }
    }
    ofsl_dir_list_end(finfo);

    if (subdir) {
        ofsl_dir_close(subdir);
    }

    fclose(imgtree);

    ofsl_dir_close(rootdir);
}

static void test_file_read(void)
{
    OFSL_Directory* rootdir = ofsl_fs_rootdir_open(fat);
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

static int clean_test_suite(void)
{
    ofsl_fs_delete(fat);
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
            .pName = "filesystem recognization",
            .pTestFunc = test_get_filesystem_name
        },
        {
            .pName = "directory list",
            .pTestFunc = test_dir_list
        },
        {
            .pName = "invalid file or directory",
            .pTestFunc = test_no_such_entry,
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
            .pName = "fs/fat/fat12",
            .pInitFunc = init_fat12_suite,
            .pCleanupFunc = clean_test_suite,
            .pTests = tests
        },
        {
            .pName = "fs/fat/fat12_cl",
            .pInitFunc = init_fat12_cl_suite,
            .pCleanupFunc = clean_test_suite,
            .pTests = tests
        },
        {
            .pName = "fs/fat/fat12_clu",
            .pInitFunc = init_fat12_clu_suite,
            .pCleanupFunc = clean_test_suite,
            .pTests = tests
        },
        {
            .pName = "fs/fat/fat12_csl",
            .pInitFunc = init_fat12_csl_suite,
            .pCleanupFunc = clean_test_suite,
            .pTests = tests
        },
        {
            .pName = "fs/fat/fat16",
            .pInitFunc = init_fat16_suite,
            .pCleanupFunc = clean_test_suite,
            .pTests = tests
        },
        {
            .pName = "fs/fat/fat32",
            .pInitFunc = init_fat32_suite,
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
