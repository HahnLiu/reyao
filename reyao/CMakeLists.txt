file(GLOB LIB_SRC ${PROJECT_SOURCE_DIR}/reyao/*.cc 
                  ${PROJECT_SOURCE_DIR}/reyao/http/*.cc 
                  ${PROJECT_SOURCE_DIR}/reyao/rpc/*.cc)
file(GLOB reyao_headers ${PROJECT_SOURCE_DIR}/reyao/*.h)
file(GLOB reyao_http_headers ${PROJECT_SOURCE_DIR}/reyao/http/*.h)
file(GLOB reyao_rpc_headers ${PROJECT_SOURCE_DIR}/reyao/rpc/*.h)

add_library(reyao SHARED ${LIB_SRC})

# set(variable value CACHE INTERNAL docstring )这种
SET(LIBS 
    reyao
    pthread
    dl
    ${PROTOBUF_LIBRARIES}
    ${ZLIB_LIBRARIES}
    CACHE
    INTERNAL 
    "reyao lib"
)

add_subdirectory(tests)

install(TARGETS reyao
        ARCHIVE DESTINATION /usr/lib
        LIBRARY DESTINATION /usr/lib)
install(FILES ${reyao_headers}
        DESTINATION /usr/local/include/reyao)
install(FILES ${reyao_http_headers}
        DESTINATION /usr/local/include/reyao/http)
install(FILES ${reyao_rpc_headers}
        DESTINATION /usr/local/include/reyao/rpc)

set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)  
