include_directories (${TEA_SOURCE_DIR}/inc) 
include_directories (${TEA_SOURCE_DIR}/inc/utils)

add_subdirectory (utils)

add_executable (tea-client
	tea-client.cc Player.cc
)
add_executable (tea-server
	tea-server.cc Client.cc Player.cc
)

target_link_libraries(tea-server
	splitstr
	netutils
)

target_link_libraries(tea-client
	${OPENGL_LIBRARY}
	${SDL_LIBRARY}
	splitstr
	netutils
)
