project(testgpumc)
set(SRCS
	TestGPUMC.h TestGPUMC.cpp
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} WINDOWED NOINSTALL)
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES testcore computevideo voxelgenerator)
generate_compute_shaders(${PROJECT_NAME} marchingcubes marchingcubes-buffer)
generate_shaders(${PROJECT_NAME} vertex)
