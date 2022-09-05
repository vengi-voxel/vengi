set(LIB voxedit-ui)

set(SRCS
	AnimationTimeline.h AnimationTimeline.cpp
	AnimationPanel.h AnimationPanel.cpp
	AssetPanel.h AssetPanel.cpp
	DragAndDropPayload.h
	PositionsPanel.h PositionsPanel.cpp
	LayerPanel.h LayerPanel.cpp
	LSystemPanel.h LSystemPanel.cpp
	MainWindow.h MainWindow.cpp
	MenuBar.h MenuBar.cpp
	ModifierPanel.h ModifierPanel.cpp
	PalettePanel.h PalettePanel.cpp
	SceneGraphPanel.h SceneGraphPanel.cpp
	ScriptPanel.h ScriptPanel.cpp
	StatusBar.h StatusBar.cpp
	ToolsPanel.h ToolsPanel.cpp
	TreePanel.h TreePanel.cpp
	Util.h Util.cpp
	Viewport.h Viewport.cpp
)

engine_add_module(TARGET ${LIB} SRCS ${SRCS} FILES ${FILES} DEPENDENCIES imgui voxedit-util)

if (VOXEDIT_ANIMATION)
	target_compile_definitions(${LIB} PUBLIC VOXEDIT_ANIMATION)
endif()
