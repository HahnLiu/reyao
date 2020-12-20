# 查找所有源文件和头文件
file(GLOB LIB_SRC ${PROJECT_SOURCE_DIR}/reyao/*.cc ${PROJECT_SOURCE_DIR}/reyao/http/*.cc)
file(GLOB reyao_headers ${PROJECT_SOURCE_DIR}/reyao/*.h)
file(GLOB reyao_http_headers ${PROJECT_SOURCE_DIR}/reyao/http/*.h)

# 用源文件生成动态库
add_library(reyao SHARED ${LIB_SRC})

# 添加项目用到的所有库
SET(LIBS 
    reyao
    pthread
    dl
    # ${PROTOBUF_LIBRARIES}
    # ${ZLIB_LIBRARIES}
)

# set(REYAO_LIBRARY ${LIBS} CACHE INTERNAL "Reyao Library" )

add_subdirectory(tests)

# 安装动态库与头文件
install(TARGETS reyao
        ARCHIVE DESTINATION /usr/lib
        LIBRARY DESTINATION /usr/lib)
install(FILES ${reyao_headers}
        DESTINATION /usr/local/include/reyao)
install(FILES ${reyao_http_headers}
        DESTINATION /usr/local/include/reyao/http)

# 如果是内部编译，PROJECT_BINARY_DIR在编译发生的当前目录，如果是外部编译，则是在外部编译的目录
# 在哪里add_library，就在哪里的CMakeLists.txt中设置LIBRARY_OUTPUT_PATH
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)  #项目生成态库输出目录
