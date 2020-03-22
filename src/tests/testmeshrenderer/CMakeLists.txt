project(testmeshrenderer)
set(SRCS
	TestMeshRenderer.h TestMeshRenderer.cpp
)
set(FILES
	voxel/assets/north-dir.vox
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} FILES ${FILES} WINDOWED NOINSTALL)
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES voxelrender testcore)
