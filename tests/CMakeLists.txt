cmake_minimum_required(VERSION 3.13)

include (CTest)

find_package(PkgConfig)
pkg_check_modules(CUnit REQUIRED cunit)

set(TEST_CFLAGS ${CUnit_CFLAGS})
set(TEST_LDFLAGS ${CUnit_LDFLAGS})

add_subdirectory(drive)
add_test(NAME test_rawimage
    COMMAND $<TARGET_FILE:test_rawimage>
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

add_subdirectory(fs)
add_test(NAME test_fat
    COMMAND $<TARGET_FILE:test_fat>
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

add_subdirectory(ptbl)
add_test(NAME test_gpt
    COMMAND $<TARGET_FILE:test_gpt>
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
add_test(NAME test_mbr
    COMMAND $<TARGET_FILE:test_mbr>
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
