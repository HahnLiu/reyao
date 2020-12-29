cmake_minimum_required(VERSION 2.8)
project(reyao)

# 显示编译信息
# set(CMAKE_VERBOSE_MAKEFILE ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "DEBUG")
endif()

# 添加编译选项
#####################################################################
set(CXX_FLAGS 
    #  -ggdb 
     -Wall
     -Werror
     -Wno-unused-result
     -std=c++11
     -rdynamic
     -fPIC
    #  -O3
)

# CXX_FLAGS:-ggdb;-Wall;-Werror;-std=c++11;-rdynamic;-fPIC;-O3
# message(STATUS "CXX_FLAGS:${CXX_FLAGS}")

# usage: string(replace match_str replace_str output_str input_str)
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")
#######################################################################

# 添加头文件查找路径
#######################################################################
include_directories(.)

# 查找依赖库并引入依赖库的头文件
# find_package(Protobuf)
# if (Protobuf_FOUND)
#     include_directories(${Protobuf_INCLUDE_DIRS})
# endif()

# find_package(ZLIB REQUIRED)
# if (ZLIB_FOUND)
#     include_directories(${ZLIB_INCLUDE_DIRS})
# endif()
########################################################################

# usage: string(toupper intput_str output_str)
# string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE)

#添加源文件子目录
add_subdirectory(reyao)

set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)  #项目生成态库输出目录
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)  #项目的可执行文件输出目录

