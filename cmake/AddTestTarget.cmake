cmake_minimum_required(VERSION 3.13)

include (CTest)

find_package(PkgConfig)
pkg_check_modules(CUnit REQUIRED cunit)

function(add_test_target test_target_name test_target_sources)
    list(SUBLIST ARGV 1 -1 test_target_sources)

    add_executable(${test_target_name} ${test_target_sources})
    target_include_directories(${test_target_name} PUBLIC "${CMAKE_SOURCE_DIR}/include")
    target_include_directories(${test_target_name} PUBLIC "${CUnit_INCLUDE_DIRS}")
    target_compile_options(${test_target_name} PUBLIC "${CUnit_CFLAGS_OTHER}")

    target_link_libraries(${test_target_name} PUBLIC openfsl2)
    target_link_libraries(${test_target_name} PUBLIC "${CUnit_LIBRARIES}")
    target_link_directories(${test_target_name} PUBLIC "${CUnit_LIBRARY_DIRS}")
    target_link_options(${test_target_name} PUBLIC "${CUnit_LDFLAGS_OTHER}")
    add_test(NAME ${test_target_name}
        COMMAND $<TARGET_FILE:${test_target_name}>
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
endfunction()
