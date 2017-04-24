project(testdepthbuffer)
set(SRCS
	TestDepthBuffer.h TestDepthBuffer.cpp
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} WINDOWED)
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES testcore)
