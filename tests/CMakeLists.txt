cmake_minimum_required(VERSION 3.13)

include (CTest)

find_package(PkgConfig)
pkg_check_modules(CUnit REQUIRED cunit)

set(TEST_CFLAGS ${CUnit_CFLAGS})
set(TEST_LDFLAGS ${CUnit_LDFLAGS})

add_subdirectory(general)
add_subdirectory(drive)
add_subdirectory(fs)
add_subdirectory(partition)
