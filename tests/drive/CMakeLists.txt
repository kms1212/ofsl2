cmake_minimum_required(VERSION 3.13)

project("test_drive")

add_executable(test_rawimage test_rawimage.c)
target_include_directories(test_rawimage PUBLIC "${CMAKE_SOURCE_DIR}/include")
target_link_libraries(test_rawimage PUBLIC openfsl2)
target_link_options(test_rawimage PUBLIC ${TEST_LDFLAGS})
target_compile_options(test_rawimage PUBLIC ${TEST_CFLAGS})
