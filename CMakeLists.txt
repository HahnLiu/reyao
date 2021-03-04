cmake_minimum_required(VERSION 2.8)
project(reyao)

# 显示编译信息
# set(CMAKE_VERBOSE_MAKEFILE ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "DEBUG")
endif()

set(CXX_FLAGS 
    #  -ggdb 
     -Wall
     -Werror
     -Wno-unused-result
     -std=c++11
     -rdynamic
     -fPIC
     -O3
)
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")

# 添加头文件查找路径
#######################################################################
include_directories(.)

# 查找依赖库并引入依赖库的头文件
find_package(Protobuf)
if (Protobuf_FOUND)
    include_directories(${Protobuf_INCLUDE_DIRS})
endif()

find_package(ZLIB REQUIRED)
if (ZLIB_FOUND)
    include_directories(${ZLIB_INCLUDE_DIRS})
endif()
########################################################################

#添加源文件子目录
add_subdirectory(reyao)

