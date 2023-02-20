set(SRCS
	MementoHandler.h MementoHandler.cpp
	SceneManager.h SceneManager.cpp
	AxisUtil.h AxisUtil.cpp
	Config.h

	modifier/Modifier.h modifier/Modifier.cpp
	modifier/ModifierRenderer.h modifier/ModifierRenderer.cpp
	modifier/ModifierFacade.h modifier/ModifierFacade.cpp
	modifier/ModifierType.h
	modifier/ModifierButton.h modifier/ModifierButton.cpp
	modifier/ModifierType.h

	SceneRenderer.h SceneRenderer.cpp
	SceneUtil.h SceneUtil.cpp

	ModelNodeSettings.h

	tool/Clipboard.h tool/Clipboard.cpp
)

set(LIB voxedit-util)
set(DEPENDENCIES voxelrender voxelgenerator voxelfont)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES ${DEPENDENCIES})

set(TEST_SRCS
	tests/MementoHandlerTest.cpp
	tests/ModifierTest.cpp
	tests/SceneManagerTest.cpp
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
