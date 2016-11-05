project(voxedit)
set(SRCS
	VoxEdit.h VoxEdit.cpp
	ui/EditorScene.h ui/EditorScene.cpp
	ui/MainWindow.h ui/MainWindow.cpp
	ui/PaletteWidget.h ui/PaletteWidget.cpp
	ui/select/Select.h
	ui/select/Single.h
	voxelizer.h
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} WINDOWED)
target_link_libraries(${PROJECT_NAME} frontend util voxel)
check_ui_files(${PROJECT_NAME})
