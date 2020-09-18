set(SRCS
	PoiProvider.cpp PoiProvider.h
	Type.h
)
set(LIB poi)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES voxelworld shared)

set(TEST_SRCS
	tests/PoiProviderTest.cpp
)

gtest_suite_sources(tests ${TEST_SRCS})
gtest_suite_deps(tests ${LIB} test-app)

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_deps(tests-${LIB} ${LIB} test-app)
gtest_suite_end(tests-${LIB})
