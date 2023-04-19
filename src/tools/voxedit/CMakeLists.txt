include_directories(modules)
add_subdirectory(modules)

project(voxedit)
set(SRCS
	VoxEdit.h VoxEdit.cpp
)

set(FILES
	voxedit/scripts/test-import-plane.cfg
	voxedit/scripts/test-layers.cfg
	voxedit/scripts/test-animate.cfg
	shared/font.ttf
)

engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} FILES ${FILES} WINDOWED DESCRIPTION "Voxel Editor")
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES voxedit-ui voxedit-util)
add_dependencies(${PROJECT_NAME} codegen)
