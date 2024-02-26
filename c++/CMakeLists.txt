cmake_minimum_required(VERSION 3.13)

set(BUILD_CXX_WRAPPER TRUE CACHE BOOL "Build C++ Wrapper")
if(${BUILD_CXX_WRAPPER})
    target_compile_features(openfsl2 PUBLIC cxx_std_11)
    target_include_directories(openfsl2 PUBLIC "${CMAKE_SOURCE_DIR}/c++/include")
    target_sources(openfsl2 PRIVATE
        drive/drive.cc
        drive/rawimage.cc
        fs/fs.cc)
endif()

