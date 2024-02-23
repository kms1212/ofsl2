#include <assert.h>

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include <ofsl/drive/rawimage.h>

#define TEST_SECTOR_SIZE 512

OFSL_Drive* drive;

static int init_test_suite(void)
{
    drive = ofsl_create_rawimage_drive("tests/data/drive/rawimage.img", 0, TEST_SECTOR_SIZE, 4);
    assert(drive);
    return 0;
}

static int clean_test_suite(void)
{
    ofsl_drive_delete(drive);
    return 0;
}

static void test_create(void)
{
    /* invalid file name */
    CU_ASSERT_PTR_NULL(ofsl_create_rawimage_drive("", 0, TEST_SECTOR_SIZE, 4));

    /* invalid file size */
    CU_ASSERT_PTR_NULL(ofsl_create_rawimage_drive("tests/data/drive/rawimage.img", 0, TEST_SECTOR_SIZE, 5));
}

static void test_update_info(void)
{
    CU_ASSERT_FALSE(ofsl_drive_update_info(drive));
}

static void test_read_sector(void)
{
    uint8_t buf[TEST_SECTOR_SIZE * 2];
    CU_ASSERT_EQUAL(ofsl_drive_read_sector(drive, buf, 0, TEST_SECTOR_SIZE, 2), 2);
    CU_ASSERT_EQUAL(memcmp(buf, buf + TEST_SECTOR_SIZE + 16, 16), 0);
    CU_ASSERT_EQUAL(ofsl_drive_read_sector(drive, buf, 2, TEST_SECTOR_SIZE, 2), 2);
    CU_ASSERT_EQUAL(memcmp(buf, buf + TEST_SECTOR_SIZE + 16, 16), 0);

    /* invalid sector size */
    CU_ASSERT(ofsl_drive_read_sector(drive, buf, 0, 1024, 1) < 1);
}

static void test_write_sector(void)
{
    uint8_t buf[TEST_SECTOR_SIZE];
    const uint8_t wdata[] = {
        0xFF, 0x00, 0xEE, 0x11, 0xDD, 0x22, 0xCC, 0x33,
        0xBB, 0x44, 0xAA, 0x55, 0x99, 0x66, 0x88, 0x77,
    };
    CU_ASSERT_EQUAL(ofsl_drive_read_sector(drive, buf, 0, TEST_SECTOR_SIZE, 1), 1);
    memcpy(buf + 16, wdata, sizeof(wdata));
    CU_ASSERT_EQUAL(ofsl_drive_write_sector(drive, buf, 0, TEST_SECTOR_SIZE, 1), 1);
    CU_ASSERT_EQUAL(ofsl_drive_read_sector(drive, buf, 0, TEST_SECTOR_SIZE, 1), 1);
    CU_ASSERT_EQUAL(memcmp(buf + 16, wdata, sizeof(wdata)), 0);
    memset(buf + 16, 0, sizeof(wdata));
    CU_ASSERT_EQUAL(ofsl_drive_write_sector(drive, buf, 0, TEST_SECTOR_SIZE, 1), 1);

    /* invalid sector size */
    CU_ASSERT(ofsl_drive_write_sector(drive, buf, 0, 1024, 1) < 1);
}

int main(int argc, char** argv)
{
    CU_pSuite pSuite = NULL;

    if (CU_initialize_registry() != CUE_SUCCESS) {
        return CU_get_error();
    }

    pSuite = CU_add_suite("drive/rawimage", init_test_suite, clean_test_suite);
    if (pSuite == NULL) {
        goto error_exit;
    }

    if ((CU_add_test(pSuite, "create", test_create) == NULL) ||
        (CU_add_test(pSuite, "update info", test_update_info) == NULL) ||
        (CU_add_test(pSuite, "read sector", test_read_sector) == NULL) ||
        (CU_add_test(pSuite, "write sector", test_write_sector) == NULL)) {
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
