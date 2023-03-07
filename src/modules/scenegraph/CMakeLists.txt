set(LIB scenegraph)
set(SRCS
	SceneGraph.h SceneGraph.cpp
	SceneGraphNode.h SceneGraphNode.cpp
	SceneGraphUtil.h SceneGraphUtil.cpp
)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES voxelutil)

set(TEST_SRCS
	tests/SceneGraphTest.cpp
	tests/SceneGraphUtilTest.cpp
	tests/TestHelper.h
)

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_files(tests-${LIB} ${TEST_FILES})
gtest_suite_deps(tests-${LIB} ${LIB} test-app video)
gtest_suite_end(tests-${LIB})
