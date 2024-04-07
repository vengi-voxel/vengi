set(LIB voxedit-ui)

set(SRCS
	AnimationTimeline.h AnimationTimeline.cpp
	AnimationPanel.h AnimationPanel.cpp
	AssetPanel.h AssetPanel.cpp
	CameraPanel.h CameraPanel.cpp
	DragAndDropPayload.h
	FileDialogOptions.h FileDialogOptions.cpp
	Gizmo.h
	PositionsPanel.h PositionsPanel.cpp
	LSystemPanel.h LSystemPanel.cpp
	MainWindow.h MainWindow.cpp
	MenuBar.h MenuBar.cpp
	MementoPanel.h MementoPanel.cpp
	PalettePanel.h PalettePanel.cpp
	RenderPanel.h RenderPanel.cpp
	SceneGraphPanel.h SceneGraphPanel.cpp
	ScriptPanel.h ScriptPanel.cpp
	BrushPanel.h BrushPanel.cpp
	StatusBar.h StatusBar.cpp
	TipOfTheDay.h
	ToolsPanel.h ToolsPanel.cpp
	TreePanel.h TreePanel.cpp
	Util.h Util.cpp
	Viewport.h Viewport.cpp
	QuitDisallowReason.h
	WindowTitles.h
)

if (USE_IMGUITESTENGINE)
	list(APPEND SRCS
		tests/AnimationPanelTest.cpp
		tests/AnimationTimelineTest.cpp
		tests/AssetPanelTest.cpp
		tests/CameraPanelTest.cpp
		tests/PositionsPanelTest.cpp
		tests/LSystemPanelTest.cpp
		tests/MainWindowTest.cpp
		tests/MenuBarTest.cpp
		tests/MementoPanelTest.cpp
		tests/PalettePanelTest.cpp
		tests/RenderPanelTest.cpp
		tests/SceneGraphPanelTest.cpp
		tests/ScriptPanelTest.cpp
		tests/BrushPanelTest.cpp
		tests/StatusBarTest.cpp
		tests/ToolsPanelTest.cpp
		tests/TreePanelTest.cpp
		tests/ViewportTest.cpp
	)
endif()

engine_add_module(TARGET ${LIB} SRCS ${SRCS} FILES ${FILES} DEPENDENCIES ui voxedit-util voxelpathtracer voxelcollection)
target_include_directories(${LIB} PRIVATE ${GENERATE_DIR})

# vengi model files in data/voxedit
set(VENGI_MODELS
	aquarium
	robo
	chr_blacksmith
	chr_dwarf
	chr_female
	chr_knight
	chr_knight2
	chr_man
	chr_oldman
	chess
	head
	hawk
	locomotive
	twinsen
	undead
	skeleton
	river
)
foreach (vengi_model ${VENGI_MODELS})
	set(header ${GENERATE_DIR}/${vengi_model}.h)
	set(pngheader ${GENERATE_DIR}/${vengi_model}_png.h)
	engine_file_to_header(${LIB} ${vengi_model} ${DATA_DIR}/voxedit/${vengi_model}.vengi ${header})
	engine_file_to_header(${LIB} ${vengi_model}_png ${DATA_DIR}/voxedit/${vengi_model}.png ${pngheader})
endforeach()
