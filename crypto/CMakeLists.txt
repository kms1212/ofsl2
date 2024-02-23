cmake_minimum_required(VERSION 3.13)

set(USE_ZLIB TRUE CACHE BOOL "Use zlib if available")
if(${USE_ZLIB})
    find_package(ZLIB)
endif()

if(${ZLIB_FOUND})
    target_include_directories(openfsl2 PRIVATE ${ZLIB_INCLUDE_DIRS})
    target_link_libraries(openfsl2 PRIVATE ${ZLIB_LIBRARIES})
else()
    set(USE_ZLIB FALSE)
endif()

target_sources(openfsl2 PRIVATE crc32.c)

configure_file("config.h.in" "config.h")

