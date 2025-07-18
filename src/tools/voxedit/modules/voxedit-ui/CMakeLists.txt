set(LIB voxedit-ui)

set(SRCS
	AnimationTimeline.h AnimationTimeline.cpp
	AnimationPanel.h AnimationPanel.cpp
	AssetPanel.h AssetPanel.cpp
	CameraPanel.h CameraPanel.cpp
	CollectionPanel.h CollectionPanel.cpp
	DragAndDropPayload.h
	Gizmo.h
	ViewMode.h
	NodeInspectorPanel.h NodeInspectorPanel.cpp
	NodePropertiesPanel.h NodePropertiesPanel.cpp
	LSystemPanel.h LSystemPanel.cpp
	MainWindow.h MainWindow.cpp
	MenuBar.h MenuBar.cpp
	MementoPanel.h MementoPanel.cpp
	NormalPalettePanel.h NormalPalettePanel.cpp
	PalettePanel.h PalettePanel.cpp
	RenderPanel.h RenderPanel.cpp
	SceneDebugPanel.h SceneDebugPanel.cpp
	SceneGraphPanel.h SceneGraphPanel.cpp
	ScriptPanel.h ScriptPanel.cpp
	BrushPanel.h BrushPanel.cpp
	StatusBar.h StatusBar.cpp
	TipOfTheDay.h
	ToolsPanel.h ToolsPanel.cpp
	TreePanel.h TreePanel.cpp
	ViewMode.h ViewMode.cpp
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
		tests/CollectionPanelTest.cpp
		tests/NodeInspectorPanelTest.cpp
		tests/NodePropertiesPanelTest.cpp
		tests/LSystemPanelTest.cpp
		tests/MainWindowTest.cpp
		tests/MementoPanelTest.cpp
		tests/NormalPalettePanelTest.cpp
		tests/PalettePanelTest.cpp
		tests/RenderPanelTest.cpp
		tests/SceneGraphPanelTest.cpp
		tests/ScriptPanelTest.cpp
		tests/BrushPanelTest.cpp
		tests/StatusBarTest.cpp
		tests/ToolsPanelTest.cpp
		tests/TreePanelTest.cpp
		tests/ViewportTest.cpp

		tests/TestUtil.h tests/TestUtil.cpp
	)
endif()

engine_add_module(TARGET ${LIB} SRCS ${SRCS} FILES ${FILES} DEPENDENCIES voxedit-util voxelpathtracer voxelcollection voxelui)
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
engine_generated_library(vengi-models)
foreach (vengi_model ${VENGI_MODELS})
	engine_file_to_header(vengi-models ${vengi_model} ${DATA_DIR}/voxedit/${vengi_model}.vengi ${GENERATE_DIR}/${vengi_model}.h)
	engine_file_to_header(vengi-models ${vengi_model}_png ${DATA_DIR}/voxedit/${vengi_model}.png ${GENERATE_DIR}/${vengi_model}_png.h)
endforeach()
target_link_libraries(${LIB} PRIVATE vengi-models)
