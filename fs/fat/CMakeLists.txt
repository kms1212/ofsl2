cmake_minimum_required(VERSION 3.13)

include(AssertTypeSize)

set(CMAKE_EXTRA_INCLUDE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/internal.h")
set(CMAKE_REQUIRED_INCLUDES "${CMAKE_SOURCE_DIR}/include")

assert_type_size("union fat_time" UNION_FAT_TIME 2)
assert_type_size("union fat_date" UNION_FAT_DATE 2)
assert_type_size("struct fat_common_bpb" STRUCT_FAT_COMMON_BPB 36)
assert_type_size("struct fat1216_bpb" STRUCT_FAT1216_BPB 512)
assert_type_size("struct fat32_bpb" STRUCT_FAT32_BPB 512)
assert_type_size("struct fat_fsinfo" STRUCT_FAT_FSINFO 512)
assert_type_size("union fat_dir_entry" UNION_FAT_DIR_ENTRY 32)

set(CMAKE_REQUIRED_INCLUDES "")
set(CMAKE_EXTRA_INCLUDE_FILES "")

target_sources(openfsl2 PRIVATE fat.c)
