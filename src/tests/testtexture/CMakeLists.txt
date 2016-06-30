project(testtexture)
set(SRCS
	TestTexture.h TestTexture.cpp
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} WINDOWED)
target_link_libraries(${PROJECT_NAME} testcore)
generate_shaders(${PROJECT_NAME} color texture)
