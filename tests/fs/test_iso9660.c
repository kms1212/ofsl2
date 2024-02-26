#include <assert.h>
#include <stdlib.h>
#include <ctype.h>

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

    isofs = ofsl_fs_iso9660_create(&part, 0);
    assert(isofs);

    fsname_expected = "ISO9660";
    imgtree_path = "tests/data/iso9660/image-tree.txt";
    return 0;
}

static void test_mount(void)
{
    CU_ASSERT_FALSE(ofsl_fs_mount(isofs));
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
