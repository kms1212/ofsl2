cmake_minimum_required(VERSION 3.13)

project("test_fs")

add_custom_command(
    OUTPUT
        "${CMAKE_SOURCE_DIR}/tests/data/fat/fat12-tree.txt"
        "${CMAKE_SOURCE_DIR}/tests/data/fat/fat12cl-tree.txt"
        "${CMAKE_SOURCE_DIR}/tests/data/fat/fat12clu-tree.txt"
        "${CMAKE_SOURCE_DIR}/tests/data/fat/fat12csl-tree.txt"
        "${CMAKE_SOURCE_DIR}/tests/data/fat/fat16-tree.txt"
        "${CMAKE_SOURCE_DIR}/tests/data/fat/fat32-tree.txt"
        "${CMAKE_SOURCE_DIR}/tests/data/fat/fat12.img"
        "${CMAKE_SOURCE_DIR}/tests/data/fat/fat16.img"
        "${CMAKE_SOURCE_DIR}/tests/data/fat/fat32.img"
        "${CMAKE_SOURCE_DIR}/tests/data/fat/file.bin"
        "${CMAKE_SOURCE_DIR}/tests/data/fat/longfilename.bin"
        "${CMAKE_SOURCE_DIR}/tests/data/fat/유니코드.bin"
    COMMAND "${CMAKE_SOURCE_DIR}/tests/data/fat/make_data"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/tests/data/fat")
add_custom_target(test_fat_data DEPENDS 
    "${CMAKE_SOURCE_DIR}/tests/data/fat/fat12-tree.txt"
    "${CMAKE_SOURCE_DIR}/tests/data/fat/fat12cl-tree.txt"
    "${CMAKE_SOURCE_DIR}/tests/data/fat/fat12clu-tree.txt"
    "${CMAKE_SOURCE_DIR}/tests/data/fat/fat12csl-tree.txt"
    "${CMAKE_SOURCE_DIR}/tests/data/fat/fat16-tree.txt"
    "${CMAKE_SOURCE_DIR}/tests/data/fat/fat32-tree.txt"
    "${CMAKE_SOURCE_DIR}/tests/data/fat/fat12.img"
    "${CMAKE_SOURCE_DIR}/tests/data/fat/fat16.img"
    "${CMAKE_SOURCE_DIR}/tests/data/fat/fat32.img"
    "${CMAKE_SOURCE_DIR}/tests/data/fat/file.bin"
    "${CMAKE_SOURCE_DIR}/tests/data/fat/longfilename.bin"
    "${CMAKE_SOURCE_DIR}/tests/data/fat/유니코드.bin")

add_executable(test_fat test_fat.c md5.c)
add_dependencies(test_fat test_fat_data)
target_include_directories(test_fat PUBLIC "${CMAKE_SOURCE_DIR}/include")
target_link_libraries(test_fat PUBLIC openfsl2)
target_link_options(test_fat PUBLIC ${TEST_LDFLAGS})
target_compile_options(test_fat PUBLIC ${TEST_CFLAGS})
