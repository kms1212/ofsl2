cmake_minimum_required(VERSION 3.13)


function(add_test_target test_target_name test_target_sources)
    list(SUBLIST ARGV 1 -1 test_target_sources)

    add_executable(${test_target_name} ${test_target_sources})
    target_include_directories(${test_target_name} PUBLIC "${CMAKE_SOURCE_DIR}/include")
    target_link_libraries(${test_target_name} PUBLIC openfsl2)
    target_link_options(${test_target_name} PUBLIC ${TEST_LDFLAGS})
    target_compile_options(${test_target_name} PUBLIC ${TEST_CFLAGS})
    add_test(NAME ${test_target_name}
        COMMAND $<TARGET_FILE:${test_target_name}>
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
endfunction()
