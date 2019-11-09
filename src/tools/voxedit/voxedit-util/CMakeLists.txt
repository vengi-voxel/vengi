set(SRCS
	anim/VolumeCache.h anim/VolumeCache.cpp

	MementoHandler.h MementoHandler.cpp
	SceneManager.h SceneManager.cpp
	ViewportController.h ViewportController.cpp

	CustomBindingContext.h

	Modifier.h Modifier.cpp
	ModifierType.h
	ModifierButton.h ModifierButton.cpp

	LayerManager.h LayerManager.cpp
	LayerListener.h
	Layer.h

	AxisUtil.h AxisUtil.cpp
	ModifierType.h
	Config.h

	tool/ImageUtils.h tool/ImageUtils.cpp
	tool/Fill.h tool/Fill.cpp
	tool/Resize.h tool/Resize.cpp
)
set(LIB voxedit-util)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES voxelrender voxelformat voxelgenerator animation)

set(TEST_SRCS
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
gtest_suite_deps(tests ${LIB})
gtest_suite_files(tests ${TEST_FILES})

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS} ../../../modules/core/tests/AbstractTest.cpp)
gtest_suite_files(tests-${LIB} ${TEST_FILES})
gtest_suite_deps(tests-${LIB} ${LIB})
gtest_suite_end(tests-${LIB})
