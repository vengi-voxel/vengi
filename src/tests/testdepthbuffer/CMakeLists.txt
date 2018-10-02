project(testdepthbuffer)
set(SRCS
	TestDepthBuffer.h TestDepthBuffer.cpp
)
set(FILES
	testmesh/chr_skelett.fbx
	testmesh/chr_skelett2.png
)
engine_add_executable(TARGET ${PROJECT_NAME} FILES ${FILES} SRCS ${SRCS} WINDOWED NOINSTALL)
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES testcore)
