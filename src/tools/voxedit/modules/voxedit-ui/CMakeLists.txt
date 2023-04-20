set(LIB voxedit-ui)

set(SRCS
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
	QuitDisallowReason.h
)

# vengi model files in data/voxedit
set(VENGI_MODELS
	robo
	chr_knight
	chr_dwarf
	chess
	head
	twinsen
)
foreach (vengi_model ${VENGI_MODELS})
	add_custom_command(
		OUTPUT ${GENERATE_DIR}/${vengi_model}.png
		COMMAND thumbnailer --size 128 ${DATA_DIR}/voxedit/${vengi_model}.vengi ${GENERATE_DIR}/${vengi_model}.png
		DEPENDS ${DATA_DIR}/voxedit/${vengi_model}.vengi
	)
	set_source_files_properties(${GENERATE_DIR}/${vengi_model}.png PROPERTIES GENERATED TRUE)
	set(header ${GENERATE_DIR}/${vengi_model}.h)
	set(pngheader ${GENERATE_DIR}/${vengi_model}_png.h)
	engine_file_to_header(${vengi_model} ${DATA_DIR}/voxedit/${vengi_model}.vengi ${header})
	engine_file_to_header(${vengi_model}_png ${GENERATE_DIR}/${vengi_model}.png ${pngheader})
	list(APPEND SRCS ${header} ${pngheader})
endforeach()

engine_add_module(TARGET ${LIB} SRCS ${SRCS} FILES ${FILES} DEPENDENCIES ui voxedit-util)
target_include_directories(${LIB} PRIVATE ${GENERATE_DIR})
