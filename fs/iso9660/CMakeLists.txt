cmake_minimum_required(VERSION 3.13)

include(AssertTypeSize)

set(CMAKE_EXTRA_INCLUDE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/internal.h")
set(CMAKE_REQUIRED_INCLUDES "${CMAKE_SOURCE_DIR}/include")

# test size of internal structs
assert_type_size("struct biendian_pair_uint16" STRUCT_BIENDIAN_PAIR_UINT16 4)
assert_type_size("struct biendian_pair_int16" STRUCT_BIENDIAN_PAIR_INT16 4)
assert_type_size("struct biendian_pair_uint32" STRUCT_BIENDIAN_PAIR_UINT32 8)
assert_type_size("struct biendian_pair_int32" STRUCT_BIENDIAN_PAIR_INT32 8)
assert_type_size("struct isofs_vol_datetime" STRUCT_ISOFS_VOL_DATETIME 17)
assert_type_size("struct isofs_entry_datetime" STRUCT_ISOFS_ENTRY_DATETIME 7)
assert_type_size("struct isofs_dir_entry_header" STRUCT_ISOFS_DIR_ENTRY_HEADER 33)
assert_type_size("struct isofs_vol_desc" STRUCT_ISOFS_VOL_DESC 2048)

set(CMAKE_REQUIRED_INCLUDES "")
set(CMAKE_EXTRA_INCLUDE_FILES "")

target_sources(openfsl2 PRIVATE iso9660.c)
