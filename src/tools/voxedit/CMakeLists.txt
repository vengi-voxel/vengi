add_subdirectory(voxedit-util)

project(voxedit)
set(SRCS
	ui/editorscene/EditorScene.h ui/editorscene/EditorScene.cpp
	ui/editorscene/Model.h ui/editorscene/Model.cpp
	ui/editorscene/Controller.h ui/editorscene/Controller.cpp
	ui/editorscene/Action.h
	ui/editorscene/voxelizer.h

	ui/palette/PaletteWidget.h ui/palette/PaletteWidget.cpp

	ui/lsystem/RuleItem.h
	ui/lsystem/RuleItemSource.cpp
	ui/lsystem/RuleItemSource.h
	ui/lsystem/RuleItemWidget.cpp
	ui/lsystem/RuleItemWidget.h
	ui/lsystem/SyntaxHighlighter.h

	ui/VoxEditWindow.h ui/VoxEditWindow.cpp
	ui/LSystemWindow.h ui/LSystemWindow.cpp
	ui/TreeWindow.h ui/TreeWindow.cpp
	ui/NoiseWindow.h ui/NoiseWindow.cpp

	VoxEdit.h VoxEdit.cpp
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} WINDOWED)
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES turbobadger voxedit-util nativefiledialog)
include_directories(.)
check_ui_turbobadger(${PROJECT_NAME})
