cmake_minimum_required(VERSION 3.13)

project("test_ptbl")

add_custom_command(
    OUTPUT
        "${CMAKE_SOURCE_DIR}/tests/data/ptbl/mbr.img"
        "${CMAKE_SOURCE_DIR}/tests/data/ptbl/gpt.img"
    COMMAND "${CMAKE_SOURCE_DIR}/tests/data/ptbl/make_data"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/tests/data/ptbl")
add_custom_target(test_gpt_data DEPENDS 
    "${CMAKE_SOURCE_DIR}/tests/data/ptbl/gpt.img")
add_custom_target(test_mbr_data DEPENDS 
    "${CMAKE_SOURCE_DIR}/tests/data/ptbl/mbr.img")

add_executable(test_gpt test_gpt.c)
add_dependencies(test_gpt test_gpt_data)
target_include_directories(test_gpt PUBLIC "${CMAKE_SOURCE_DIR}/include")
target_link_libraries(test_gpt PUBLIC openfsl2)
target_link_options(test_gpt PUBLIC ${TEST_LDFLAGS})
target_compile_options(test_gpt PUBLIC ${TEST_CFLAGS})

add_executable(test_mbr test_mbr.c)
add_dependencies(test_mbr test_mbr_data)
target_include_directories(test_mbr PUBLIC "${CMAKE_SOURCE_DIR}/include")
target_link_libraries(test_mbr PUBLIC openfsl2)
target_link_options(test_mbr PUBLIC ${TEST_LDFLAGS})
target_compile_options(test_mbr PUBLIC ${TEST_CFLAGS})
