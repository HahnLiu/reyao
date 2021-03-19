add_executable(thread_test thread_test.cc)
target_link_libraries(thread_test ${LIBS})

add_executable(asynclog_test asynclog_test.cc)
target_link_libraries(asynclog_test ${LIBS})

add_executable(log_test log_test.cc)
target_link_libraries(log_test ${LIBS})

add_executable(scheduler_test scheduler_test.cc)
target_link_libraries(scheduler_test ${LIBS})

add_executable(hook_test hook_test.cc)
target_link_libraries(hook_test ${LIBS})

add_executable(address_test address_test.cc)
target_link_libraries(address_test ${LIBS})

add_executable(bytearray_test bytearray_test.cc)
target_link_libraries(bytearray_test ${LIBS})

add_executable(tcp_server_test tcp_server_test.cc)
target_link_libraries(tcp_server_test ${LIBS})

add_executable(echo_server echo_server.cc)
target_link_libraries(echo_server ${LIBS})

add_executable(http_session_test http_session_test.cc)
target_link_libraries(http_session_test ${LIBS})

add_executable(http_server_test http_server_test.cc)
target_link_libraries(http_server_test ${LIBS})

add_executable(tcp_client_test tcp_client_test.cc)
target_link_libraries(tcp_client_test ${LIBS})

add_executable(echo_service echo_service.cc echo.pb.cc)
target_link_libraries(echo_service ${LIBS})

add_executable(echo_client echo_client.cc echo.pb.cc)
target_link_libraries(echo_client ${LIBS})

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)  #项目的可执行文件输出目录
