set(LIB voxedit-ui)

set(SRCS
	AnimationPanel.h AnimationPanel.cpp
	CursorPanel.h CursorPanel.cpp
	FormatSettingsPanel.h FormatSettingsPanel.cpp
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
