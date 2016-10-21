project(voxedit)
set(SRCS
	Actions.h Actions.cpp
	VoxEdit.h VoxEdit.cpp
	ui/ToolWindow.h
	ui/VoxEditUI.h ui/VoxEditUI.cpp
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} WINDOWED)
target_link_libraries(${PROJECT_NAME} frontend util voxel)
check_ui_files(${PROJECT_NAME})
