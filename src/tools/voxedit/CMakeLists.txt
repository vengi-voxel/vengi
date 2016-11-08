project(voxedit)
set(SRCS
	ui/editorscene/EditorScene.h ui/editorscene/EditorScene.cpp
	ui/editorscene/Model.h ui/editorscene/Model.cpp
	ui/editorscene/Controller.h ui/editorscene/Controller.cpp
	ui/editorscene/Action.h
	ui/editorscene/Shape.h
	ui/editorscene/Axis.h
	ui/editorscene/voxelizer.h
	ui/editorscene/select/Edge.h ui/editorscene/select/Edge.cpp
	ui/editorscene/select/LineHorizontal.h ui/editorscene/select/LineHorizontal.cpp
	ui/editorscene/select/LineVertical.h ui/editorscene/select/LineVertical.cpp
	ui/editorscene/select/Same.h ui/editorscene/select/Same.cpp
	ui/editorscene/select/Single.h
	ui/editorscene/select/Select.h ui/editorscene/select/Select.cpp
	ui/editorscene/SelectType.h

	ui/palette/PaletteWidget.h ui/palette/PaletteWidget.cpp

	ui/MainWindow.h ui/MainWindow.cpp

	VoxEdit.h VoxEdit.cpp
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} WINDOWED)
target_link_libraries(${PROJECT_NAME} frontend util voxel)
check_ui_files(${PROJECT_NAME})
