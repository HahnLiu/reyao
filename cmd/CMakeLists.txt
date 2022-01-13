add_executable(http_server http_server.cc)
target_link_libraries(http_server ${LIBS})

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)  