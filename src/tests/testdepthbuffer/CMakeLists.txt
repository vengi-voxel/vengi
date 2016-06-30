project(testdepthbuffer)
set(SRCS
	TestDepthBuffer.h TestDepthBuffer.cpp
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} WINDOWED)
target_link_libraries(${PROJECT_NAME} testcore)
