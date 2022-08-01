set(SRCS
	MementoHandler.h MementoHandler.cpp
	SceneManager.h SceneManager.cpp
	CustomBindingContext.h
	AxisUtil.h AxisUtil.cpp
	Config.h

	ViewportController.h ViewportController.cpp

	modifier/Modifier.h modifier/Modifier.cpp
	modifier/ModifierRenderer.h modifier/ModifierRenderer.cpp
	modifier/ModifierFacade.h modifier/ModifierFacade.cpp
	modifier/ModifierType.h
	modifier/ModifierButton.h modifier/ModifierButton.cpp
	modifier/ModifierType.h

	layer/LayerSettings.h

	tool/Clipboard.h tool/Clipboard.cpp
)
if (VOXEDIT_ANIMATION)
	list(APPEND SRCS
		anim/VolumeCache.h anim/VolumeCache.cpp
		anim/AnimationLuaSaver.h anim/AnimationLuaSaver.cpp
	)
endif()

set(LIB voxedit-util)
set(DEPENDENCIES voxelrender voxelgenerator)
if (VOXEDIT_ANIMATION)
	list(APPEND DEPENDENCIES animation)
endif()
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES ${DEPENDENCIES})

if (VOXEDIT_ANIMATION)
	target_compile_definitions(${LIB} PUBLIC VOXEDIT_ANIMATION)
endif()

set(TEST_SRCS
	tests/MementoHandlerTest.cpp
	tests/ModifierTest.cpp
	tests/SceneManagerTest.cpp
)
if (VOXEDIT_ANIMATION)
	list(APPEND TEST_SRCS
		tests/AnimationLuaSaverTest.cpp
	)
endif()

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
