cmake_minimum_required(VERSION 3.13)

include(AddTestTarget)

project("test_general")

add_test_target(test_time test_time.c)
