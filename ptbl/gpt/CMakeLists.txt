cmake_minimum_required(VERSION 3.13)

include(AssertTypeSize)

set(CMAKE_EXTRA_INCLUDE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/internal.h")
set(CMAKE_REQUIRED_INCLUDES "${CMAKE_SOURCE_DIR}/include")

assert_type_size("struct mbr_part_entry" STRUCT_MBR_PART_ENTRY 16)
assert_type_size("struct mbr_table_sector" STRUCT_MBR_TABLE_SECTOR 512)
assert_type_size("struct gpt_table_header" STRUCT_GPT_TABLE_HEADER 512)
assert_type_size("struct gpt_part_entry" STRUCT_GPT_PART_ENTRY 128)

set(CMAKE_REQUIRED_INCLUDES "")
set(CMAKE_EXTRA_INCLUDE_FILES "")

target_sources(openfsl2 PRIVATE gpt.c)
