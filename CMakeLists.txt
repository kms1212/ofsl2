cmake_minimum_required(VERSION 3.13)
cmake_policy(SET CMP0076 NEW)
include(CheckSymbolExists)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

project("openfsl2")

file(GLOB COMMON_SOURCES "${CMAKE_SOURCE_DIR}/src/*")
set(LIBRARY_TYPE SHARED CACHE STRING "Library type (SHARED or STATIC)")
add_library(openfsl2 SHARED ${COMMON_SOURCES})

add_subdirectory(fs)
add_subdirectory(ptbl)
add_subdirectory(crypto)
add_subdirectory(fsal)

target_compile_features(openfsl2 PUBLIC c_std_99)
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

if(${BUILD_TESTING})
    add_subdirectory(tests)
    set(GENERATE_COVERAGE FALSE CACHE BOOL "Generate coverage report of tests")
    if(${GENERATE_COVERAGE})
        target_compile_options(openfsl2 PRIVATE $<$<COMPILE_LANGUAGE:C,CXX>:--coverage>)
        target_link_options(openfsl2 PRIVATE $<$<COMPILE_LANGUAGE:C,CXX>:--coverage>)
    endif()
endif()
