project(voxedit)
set(SRCS
	ui/editorscene/EditorModel.h ui/editorscene/EditorModel.cpp
	ui/editorscene/EditorScene.h ui/editorscene/EditorScene.cpp
	ui/editorscene/select/Select.h
	ui/editorscene/select/Single.h
	ui/editorscene/SelectionType.h
	ui/editorscene/Action.h
	ui/editorscene/voxelizer.h

	ui/palette/PaletteWidget.h ui/palette/PaletteWidget.cpp

	ui/MainWindow.h ui/MainWindow.cpp

	VoxEdit.h VoxEdit.cpp
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} WINDOWED)
target_link_libraries(${PROJECT_NAME} frontend util voxel)
check_ui_files(${PROJECT_NAME})
