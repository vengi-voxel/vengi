set(LIB physic)
set(SRCS
	PhysicMgr.cpp PhysicMgr.h

	VoxTreeNode.h
	VoxTree.h
)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES core bullet voxel)

set(TEST_SRCS
	tests/PhysicMgrTest.cpp
)

gtest_suite_sources(tests ${TEST_SRCS})
gtest_suite_deps(tests ${LIB})

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS} ../core/tests/AbstractTest.cpp)
gtest_suite_deps(tests-${LIB} ${LIB})
gtest_suite_end(tests-${LIB})
