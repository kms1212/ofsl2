cmake_minimum_required(VERSION 3.13)

target_sources(openfsl2 PRIVATE fs.c)

set(BUILD_FILESYSTEM_FAT TRUE CACHE BOOL "Build FAT12/16/32 Filesystem")
if(${BUILD_FILESYSTEM_FAT})
    add_subdirectory(fat)
endif()

set(BUILD_FILESYSTEM_ISO9660 TRUE CACHE BOOL "Build ISO9660 Filesystem")
if(${BUILD_FILESYSTEM_ISO9660})
    add_subdirectory(iso9660)
endif()
