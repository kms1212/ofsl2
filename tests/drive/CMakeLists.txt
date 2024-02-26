cmake_minimum_required(VERSION 3.13)

include(AddTestTarget)

project("test_drive")

add_test_target(test_rawimage test_rawimage.c)
