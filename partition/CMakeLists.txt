cmake_minimum_required(VERSION 3.13)

target_sources(openfsl2 PRIVATE partition.c)

set(BUILD_PTABLE_GPT TRUE CACHE BOOL "Build GUID Partition Table")
if(${BUILD_PTABLE_GPT})
    add_subdirectory(gpt)
endif()
