include_directories (${TEA_SOURCE_DIR}/inc) 

add_executable (tea-client
	tea-client.cc Player.cc utils.cc
)
add_executable (tea-server
	tea-server.cc Server.cc Client.cc Player.cc utils.cc
)

target_link_libraries(tea-server)
target_link_libraries(tea-client ${OPENGL_LIBRARY} ${SDL_LIBRARY})
