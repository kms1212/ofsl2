cmake_minimum_required(VERSION 3.13)

include(AssertTypeSize)

set(CMAKE_EXTRA_INCLUDE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/internal.h")
set(CMAKE_REQUIRED_INCLUDES "${CMAKE_SOURCE_DIR}/include")

# test size of internal structs
assert_type_size("union fat_time" UNION_FAT_TIME 2)
assert_type_size("union fat_date" UNION_FAT_DATE 2)
assert_type_size("struct fat_bpb_sector" STRUCT_FAT_BPB_SECTOR 512)
assert_type_size("struct fat_fsinfo" STRUCT_FAT_FSINFO 512)
assert_type_size("union fat_dir_entry" UNION_FAT_DIR_ENTRY 32)

set(CMAKE_REQUIRED_INCLUDES "")
set(CMAKE_EXTRA_INCLUDE_FILES "")

target_sources(openfsl2 PRIVATE fat.c)

# extensions
set(KNOWN_FILESYSTEM_FAT_EXTENSIONS LFN)
set(BUILD_FILESYSTEM_FAT_EXTENSIONS LFN CACHE STRING "FAT Extensions to Build")

foreach(extension IN LISTS BUILD_FILESYSTEM_FAT_EXTENSIONS)
    if (${extension} IN_LIST KNOWN_FILESYSTEM_FAT_EXTENSIONS)
        set(BUILD_FILESYSTEM_FAT_${extension} 1)
    else()
        message(FATAL_ERROR "Invalid extension name for FAT")
    endif()
endforeach()

configure_file("config.h.in" "config.h")
