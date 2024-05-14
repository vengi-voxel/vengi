set(SRCS
	MementoHandler.h MementoHandler.cpp
	SceneManager.h SceneManager.cpp
	AxisUtil.h AxisUtil.cpp
	Config.h

	modifier/brush/AABBBrush.cpp modifier/brush/AABBBrush.h
	modifier/brush/Brush.cpp modifier/brush/Brush.h
	modifier/brush/LineBrush.cpp modifier/brush/LineBrush.h
	modifier/brush/PaintBrush.cpp modifier/brush/PaintBrush.h
	modifier/brush/PathBrush.cpp modifier/brush/PathBrush.h
	modifier/brush/PlaneBrush.cpp modifier/brush/PlaneBrush.h
	modifier/brush/ShapeBrush.cpp modifier/brush/ShapeBrush.h
	modifier/brush/StampBrush.cpp modifier/brush/StampBrush.h
	modifier/brush/TextBrush.cpp modifier/brush/TextBrush.h

	modifier/IModifierRenderer.h
	modifier/Modifier.h modifier/Modifier.cpp
	modifier/ModifierButton.h modifier/ModifierButton.cpp
	modifier/ModifierFacade.h modifier/ModifierFacade.cpp
	modifier/ModifierRenderer.h modifier/ModifierRenderer.cpp
	modifier/ModifierType.h
	modifier/ModifierVolumeWrapper.h
	modifier/Selection.h
	modifier/ShapeType.h

	ISceneRenderer.h
	SceneRenderer.h SceneRenderer.cpp
	SceneUtil.h SceneUtil.cpp

	ModelNodeSettings.h

	Clipboard.h Clipboard.cpp
)

set(LIB voxedit-util)
set(DEPENDENCIES ui voxelrender voxelformat voxelgenerator voxelfont)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES ${DEPENDENCIES})

set(TEST_SRCS
	tests/AbstractBrushTest.cpp tests/AbstractBrushTest.h
	tests/LineBrushTest.cpp
	tests/MementoHandlerTest.cpp
	tests/ModifierTest.cpp
	tests/PaintBrushTest.cpp
	tests/PathBrushTest.cpp
	tests/PlaneBrushTest.cpp
	tests/SceneManagerTest.cpp
	tests/SceneRendererTest.cpp
	tests/ShapeBrushTest.cpp
	tests/StampBrushTest.cpp
)

gtest_suite_sources(tests
	${TEST_SRCS}
)
gtest_suite_deps(tests ${LIB} test-app)
gtest_suite_files(tests ${TEST_FILES})

if (NOT CMAKE_GENERATOR MATCHES "Xcode")
	gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
	gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
	gtest_suite_files(tests-${LIB} ${TEST_FILES})
	gtest_suite_deps(tests-${LIB} ${LIB} test-app)
	gtest_suite_end(tests-${LIB})
endif()
