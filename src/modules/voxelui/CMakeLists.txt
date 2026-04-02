set(LIB voxelui)
set(SRCS
	FileDialogOptions.h FileDialogOptions.cpp
	LUAApiWidget.h LUAApiWidget.cpp
	ScriptApi.h ScriptApi.cpp
	ScriptBrowserPanel.h ScriptBrowserPanel.cpp
)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES voxelformat voxelgenerator ui http json)
