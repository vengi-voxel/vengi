project(testdepthbuffer)
set(SRCS
	TestDepthBuffer.h TestDepthBuffer.cpp
)
set(FILES
	client/mesh/chr_knight.fbx
	client/mesh/chr_knight.png
	client/mesh/chr_knight.lua
)
engine_add_executable(TARGET ${PROJECT_NAME} FILES ${FILES} SRCS ${SRCS} WINDOWED NOINSTALL)
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES testcore)
