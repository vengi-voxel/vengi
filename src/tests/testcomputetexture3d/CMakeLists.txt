project(testcomputetexture3d)
set(SRCS
	TestComputeTexture3D.h TestComputeTexture3D.cpp
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} WINDOWED NOINSTALL)
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES testcore compute voxel)
#generate_shaders(${PROJECT_NAME} testcomputetexture3d)
generate_compute_shaders(${PROJECT_NAME} render)
