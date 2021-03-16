cmake_minimum_required(VERSION 2.8)
project(reyao)

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

include_directories(.)

find_package(Protobuf)
if (Protobuf_FOUND)
    include_directories(${Protobuf_INCLUDE_DIRS})
endif()

find_package(ZLIB REQUIRED)
if (ZLIB_FOUND)
    include_directories(${ZLIB_INCLUDE_DIRS})
endif()

add_subdirectory(reyao)

