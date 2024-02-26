cmake_minimum_required(VERSION 3.13)

include(AddTestTarget)

project("test_partition")

add_custom_command(
    OUTPUT
        "${CMAKE_SOURCE_DIR}/tests/data/partition/mbr.img"
        "${CMAKE_SOURCE_DIR}/tests/data/partition/gpt.img"
    COMMAND "${CMAKE_SOURCE_DIR}/tests/data/partition/make_data"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/tests/data/partition")
add_custom_target(test_gpt_data DEPENDS 
    "${CMAKE_SOURCE_DIR}/tests/data/partition/gpt.img")
add_custom_target(test_mbr_data DEPENDS 
    "${CMAKE_SOURCE_DIR}/tests/data/partition/mbr.img")

add_test_target(test_gpt test_gpt.c)
add_dependencies(test_gpt test_gpt_data)

#add_test_target(test_mbr test_mbr.c)
#add_dependencies(test_mbr test_mbr_data)
