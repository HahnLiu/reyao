cmake_minimum_required(VERSION 2.8)
project(qlog)

set(CXX_FLAGS 
    -ggdb 
    -Wall
    -Werror
    -Wno-unused-function
    -std=c++11
    -O3
)

string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")

file(GLOB LIB_SRC ${PROJECT_SOURCE_DIR}/*.cc)

add_library(qlog STATIC ${LIB_SRC})

add_executable(test_log test_log.cc)
target_link_libraries(test_log qlog pthread)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)