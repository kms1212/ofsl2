#include <assert.h>
#include <stdlib.h>
#include <ctype.h>

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include <ofsl/drive/rawimage.h>
#include <ofsl/partition/gpt.h>

#define TEST_SECTOR_SIZE 512

OFSL_Drive* drive;
OFSL_PartitionTable* gpt;

static int init_test_suite(void)
{
    drive = ofsl_drive_rawimage_create("tests/data/partition/gpt.img", 0, TEST_SECTOR_SIZE);
    assert(drive);

    gpt = ofsl_ptbl_gpt_create(drive);
    assert(gpt);
    return 0;
}

static void test_part_listing(void)
{
    static struct {
        const char* name;
        lba_t lba_start;
        lba_t lba_end;
    } part_data[] = {
        {
            .name = "Microsoft basic data",
            .lba_start = 2048,
            .lba_end = 4096,
        },
        {
            .name = "Microsoft basic data",
            .lba_start = 6144,
            .lba_end = 8158,
        },
    };

    OFSL_Partition* part = ofsl_ptbl_list_start(gpt);
    CU_ASSERT_PTR_NOT_NULL_FATAL(part);

    for (int i = 0; !ofsl_ptbl_list_next(part); i++) {
        CU_ASSERT_EQUAL(strncmp(part->part_name, part_data[i].name, 36), 0);
        CU_ASSERT_EQUAL(part->lba_start, part_data[i].lba_start);
        CU_ASSERT_EQUAL(part->lba_end, part_data[i].lba_end);
    }

    ofsl_ptbl_list_end(part);
}

static int clean_test_suite(void)
{
    ofsl_ptbl_delete(gpt);
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
            .pName = "partition listing",
            .pTestFunc = test_part_listing,
        },
        CU_TEST_INFO_NULL
    };

    static CU_SuiteInfo suites[] = {
        {
            .pName = "ptbl/gpt",
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
