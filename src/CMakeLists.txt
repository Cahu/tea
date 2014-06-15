add_subdirectory (utils)

add_executable (tea-client
	tea-client.cc
	shaders.cc graphics.cc geometry.cc
)
add_executable (tea-server
	tea-server.cc
)

target_link_libraries(tea-client
	Player NetGame Map
)

target_link_libraries(tea-server
	Player Client Map
	splitstr netutils playerutils
)

target_link_libraries(tea-client
	${OPENGL_LIBRARY}
	${GLEW_LIBRARY}
	${SDL_LIBRARY}
	${GLM_LIBRARY}
	splitstr
	netutils
	mapvbo
	playerutils
)
