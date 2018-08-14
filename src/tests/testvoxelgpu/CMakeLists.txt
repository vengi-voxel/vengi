project(testvoxelgpu)
set(SRCS
	TestVoxelGPU.h TestVoxelGPU.cpp
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} WINDOWED NOINSTALL)
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES testcore compute voxel)
#generate_shaders(${PROJECT_NAME} testvoxelgpu)
generate_compute_shaders(${PROJECT_NAME} mesher)
