set(SRCS
	EditorLUAGenerator.h EditorLUAGenerator.cpp
	MementoHandler.h MementoHandler.cpp
	SceneManager.h SceneManager.cpp
	SceneSettings.h
	CustomBindingContext.h
	AxisUtil.h AxisUtil.cpp
	Config.h

	AbstractViewport.h AbstractViewport.cpp
	ViewportController.h ViewportController.cpp

	anim/VolumeCache.h anim/VolumeCache.cpp
	anim/AnimationLuaSaver.h anim/AnimationLuaSaver.cpp

	modifier/Modifier.h modifier/Modifier.cpp
	modifier/ModifierRenderer.h modifier/ModifierRenderer.cpp
	modifier/ModifierFacade.h modifier/ModifierFacade.cpp
	modifier/ModifierType.h
	modifier/ModifierButton.h modifier/ModifierButton.cpp
	modifier/ModifierType.h

	layer/LayerManager.h layer/LayerManager.cpp
	layer/LayerListener.h
	layer/Layer.h
	layer/LayerSettings.h

	tool/ImageUtils.h tool/ImageUtils.cpp
	tool/Clipboard.h tool/Clipboard.cpp
	tool/Resize.h tool/Resize.cpp
)
set(LIB voxedit-util)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES voxelrender voxelgenerator animation)

set(TEST_SRCS
	tests/AnimationLuaSaverTest.cpp
	tests/LayerManagerTest.cpp
	tests/MementoHandlerTest.cpp
	tests/ModifierTest.cpp
	tests/ImageUtilsTest.cpp
)
set(TEST_FILES
	tests/test-palette-in.png
)

gtest_suite_sources(tests
	${TEST_SRCS}
)
gtest_suite_deps(tests ${LIB} test-app)
gtest_suite_files(tests ${TEST_FILES})

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_files(tests-${LIB} ${TEST_FILES})
gtest_suite_deps(tests-${LIB} ${LIB} test-app)
gtest_suite_end(tests-${LIB})
