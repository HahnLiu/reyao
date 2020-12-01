set (LIB_SRC
     log.cc
     thread.cc
     asynclog.cc
     mutex.cc
     util.cc
     coroutine.cc
     worker.cc
     epoller.cc
     workerthread.cc
     scheduler.cc
     timer.cc
     hook.cc
     fdmanager.cc
     address.cc
     socket.cc
     bytearray.cc
     http/http_request.cc
     http/http_response.cc
     http/http_session.cc
     http/http_parser.cc
     http/http_server.cc
     http/http_servlet.cc
     http/http_connection.cc
     http/http_connectionpool.cc
     socket_stream.cc
     tcp_server.cc
     uri.cc
     stackalloc.cc
     rpc/codec.cc
     rpc/rpc_server.cc
     rpc/rpc_client.cc
)

add_library(reyao SHARED ${LIB_SRC})
#add_library(starry_static STATIC ${LIB_SRC})

#find_library(pthread pthread)

#set(LIBS ${LIBS} starry)
#set(LIBS ${LIBS} ${PTHREAD})

SET(LIBS 
    reyao
    pthread
    dl
    ${PROTOBUF_LIBRARIES}
    ${ZLIB_LIBRARIES}
)

add_subdirectory(tests)

# 如果是内部编译，PROJECT_BINARY_DIR在编译发生的当前目录，如果是外部编译，则是在外部编译的目录
# 在哪里add_library，就在哪里的CMakeLists.txt中设置LIBRARY_OUTPUT_PATH
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)  #项目生成态库输出目录
