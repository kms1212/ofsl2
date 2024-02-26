cmake_minimum_required(VERSION 3.13)

include(CheckTypeSize)

function(assert_type_size type var expected)
    check_type_size("${type}" ${var})
    if(NOT ${${var}} EQUAL ${expected})
        message(FATAL_ERROR "Invalid size of ${type}: ${${var}}, expected: ${expected}")
    endif()
endfunction()
