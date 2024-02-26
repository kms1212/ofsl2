cmake_minimum_required(VERSION 3.13)
cmake_policy(SET CMP0076 NEW)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

project("openfsl2")


set(LIBRARY_TYPE "SHARED" CACHE STRING "Library type (SHARED or STATIC)")
file(GLOB COMMON_SOURCES "${CMAKE_SOURCE_DIR}/src/*")
add_library(openfsl2 ${LIBRARY_TYPE} ${COMMON_SOURCES})


# Compiler Tests
if (CMAKE_VERSION VERSION_LESS "3.20.0")
    include(TestBigEndian)
    test_big_endian(BYTE_ORDER_BIG_ENDIAN)
    if (NOT ${BYTE_ORDER_BIG_ENDIAN})
        unset(BYTE_ORDER_BIG_ENDIAN)
    endif()
else()
    if (${CMAKE_C_BYTE_ORDER} STREQUAL BIG_ENDIAN)
        set(BYTE_ORDER_BIG_ENDIAN 1)
    endif()
endif()


# Subdirectories
add_subdirectory(fs)
add_subdirectory(partition)
add_subdirectory(crypto)
add_subdirectory(fsal)
add_subdirectory(drive)
add_subdirectory(c++)


# Compile Options
set(FORCE_STD_C99 FALSE CACHE BOOL "Force C99 Standard")
if(c_std_11 IN_LIST CMAKE_C_COMPILE_FEATURES AND NOT ${FORCE_STD_C99})
    target_compile_features(openfsl2 PUBLIC c_std_11)
else()
    target_compile_features(openfsl2 PUBLIC c_std_99)
endif()
target_compile_options(openfsl2 PUBLIC 
    $<$<COMPILE_LANGUAGE:C,CXX>:
        -Werror
        -Wall
        -Wno-unused-function
        -Wno-unused-variable
        -Wno-unused-but-set-variable
        -pedantic
        -pedantic-errors>)

target_include_directories(openfsl2 PUBLIC "${CMAKE_SOURCE_DIR}/include")
target_include_directories(openfsl2 PRIVATE "${CMAKE_SOURCE_DIR}")
target_include_directories(openfsl2 PRIVATE "${CMAKE_BINARY_DIR}")


# External Dependencies
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

configure_file("config.h.in" "config.h")


# Tests
if(${BUILD_TESTING})
    add_subdirectory(tests)
    set(GENERATE_COVERAGE FALSE CACHE BOOL "Generate coverage report of tests")
    if(${GENERATE_COVERAGE})
        target_compile_options(openfsl2 PRIVATE $<$<COMPILE_LANGUAGE:C,CXX>:--coverage>)
        target_link_options(openfsl2 PRIVATE $<$<COMPILE_LANGUAGE:C,CXX>:--coverage>)
    endif()
endif()
