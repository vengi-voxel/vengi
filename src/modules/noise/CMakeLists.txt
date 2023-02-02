set(SRCS
	Simplex.h
	Noise.h Noise.cpp
)

set(LIB noise)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES core)

set(TEST_SRCS
	tests/NoiseTest.cpp
)
gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_deps(tests-${LIB} ${LIB} test-app image)
gtest_suite_end(tests-${LIB})
