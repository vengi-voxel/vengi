project(testmesh)
set(SRCS
	TestMesh.h TestMesh.cpp
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} WINDOWED)
target_link_libraries(${PROJECT_NAME} video)
generate_shaders(${PROJECT_NAME} mesh color)
