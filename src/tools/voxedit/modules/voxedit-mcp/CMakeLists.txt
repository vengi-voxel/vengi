set(SRCS
	Tool.h Tool.cpp
	AnimationAddTool.h AnimationAddTool.cpp
	AnimationSetTool.h AnimationSetTool.cpp
	BrushTool.h BrushTool.cpp
	ShapeBrushTool.h ShapeBrushTool.cpp
	PaintBrushTool.h PaintBrushTool.cpp
	LineBrushTool.h LineBrushTool.cpp
	SelectBrushTool.h SelectBrushTool.cpp
	PlaneBrushTool.h PlaneBrushTool.cpp
	NodeAddKeyframeTool.h NodeAddKeyframeTool.cpp
	NodeRenameTool.h NodeRenameTool.cpp
	NodeMoveTool.h NodeMoveTool.cpp
	NodeRemoveTool.h NodeRemoveTool.cpp
	NodeAddModelTool.h NodeAddModelTool.cpp
	NodeSetPropertiesTool.h NodeSetPropertiesTool.cpp
	MementoCanUndoTool.h MementoCanUndoTool.cpp
	MementoCanRedoTool.h MementoCanRedoTool.cpp
	MementoUndoTool.h MementoUndoTool.cpp
	MementoRedoTool.h MementoRedoTool.cpp
	GetSceneStateTool.h GetSceneStateTool.cpp
	GetVoxelsTool.h GetVoxelsTool.cpp
	PlaceVoxelsTool.h PlaceVoxelsTool.cpp
	GetPaletteTool.h GetPaletteTool.cpp
	FindColorTool.h FindColorTool.cpp
	ScriptApiTool.h ScriptApiTool.cpp
	ScriptCreateTool.h ScriptCreateTool.cpp
	ScriptTool.h ScriptTool.cpp
	CommandTool.h CommandTool.cpp
	ToolRegistry.h ToolRegistry.cpp
)

set(LIB voxedit-mcp)
set(DEPENDENCIES voxedit-util)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES ${DEPENDENCIES})

set(TEST_SRCS
	tests/ToolsTest.cpp
)
gtest_suite_sources(tests
	${TEST_SRCS}
)
gtest_suite_deps(tests ${LIB} test-app)
gtest_suite_files(tests ${TEST_FILES})

# CMake Error: Xcode generator: Duplicate object ids not allowed
if (NOT CMAKE_GENERATOR STREQUAL "Xcode")
	gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
	gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
	gtest_suite_files(tests-${LIB} ${TEST_FILES})
	gtest_suite_deps(tests-${LIB} ${LIB} test-app)
	gtest_suite_end(tests-${LIB})
endif()
