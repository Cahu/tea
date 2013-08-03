include_directories (${TEA_SOURCE_DIR}/inc) 

add_executable (tea-client tea-client.cc Player.cc)
add_executable (tea-server tea-server.cc Server.cc Client.cc Player.cc)

target_link_libraries(tea-server ${OPENGL_LIBRARY})
target_link_libraries(tea-client ${OPENGL_LIBRARY})
