set(LIB voxedit-ui)
engine_file_to_header("robo" ${ROOT_DIR}/data/voxedit/robo.qb ${GENERATE_DIR}/robo.h)
engine_file_to_header("chr_knight" ${ROOT_DIR}/data/voxedit/chr_knight.qb ${GENERATE_DIR}/chr_knight.h)
engine_file_to_header("chess" ${ROOT_DIR}/data/voxedit/chess.vengi ${GENERATE_DIR}/chess.h)
engine_file_to_header("head" ${ROOT_DIR}/data/voxedit/head.vengi ${GENERATE_DIR}/head.h)

set(SRCS
	${GENERATE_DIR}/robo.h
	${GENERATE_DIR}/chr_knight.h
	${GENERATE_DIR}/chess.h
	${GENERATE_DIR}/head.h
	AnimationTimeline.h AnimationTimeline.cpp
	AnimationPanel.h AnimationPanel.cpp
	AssetPanel.h AssetPanel.cpp
	DragAndDropPayload.h
	FileDialogOptions.h FileDialogOptions.cpp
	PositionsPanel.h PositionsPanel.cpp
	LSystemPanel.h LSystemPanel.cpp
	MainWindow.h MainWindow.cpp
	MenuBar.h MenuBar.cpp
	MementoPanel.h MementoPanel.cpp
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

engine_add_module(TARGET ${LIB} SRCS ${SRCS} FILES ${FILES} DEPENDENCIES ui voxedit-util)
target_include_directories(${LIB} PRIVATE ${GENERATE_DIR})
