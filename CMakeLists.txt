cmake_minimum_required(VERSION 2.8)
project(reyao)

#set(CMAKE_VERBOSE_MAKEFILE ON)
# 添加编译选项
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -std=c++11 -std=c++14 -rdynamic -O3 -fPIC -Wall -Werror -ggdb")

set(CMAKE_C_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -fPIC -ggdb -std=c11 -Wall -Wno-deprecated -Werror -Wno-unused-variable -Wno-unused-result -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")

# 添加查找路径
include_directories(.)

# 查找依赖库
find_package(Protobuf)
if (Protobuf_FOUND)
    include_directories(${Protobuf_INCLUDE_DIRS})
endif()

find_package(ZLIB REQUIRED)
if (ZLIB_FOUND)
    include_directories(${ZLIB_INCLUDE_DIRS})
endif()

#set(CMAKE_CXX_COMPILER "g++")
#set(CMAKE_CXX_FLAGS_DEBUG "-O0")
#set(CMAKE_CXX_FLAGS_RELEASE "-O0")

# set(CXX_FLAGS
#     -g
#     -Wall
#     -std=c++11
#     -Wno-unused-parameter
# )


if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Reyao-Debug")
endif()

string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE)

#添加源文件子目录
add_subdirectory(reyao)

#如果是内部编译，PROJECT_BINARY_DIR在编译发生的当前目录，如果是外部编译，则是在外部编译的目录
#set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)  #项目的可执行文件输出目录
#set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)  #项目生成态库输出目录

