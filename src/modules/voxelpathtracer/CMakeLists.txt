set(LIB voxelpathtracer)
set(SRCS
	PathTracer.cpp PathTracer.h
)

engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES voxel yocto stb_image)

set(TEST_SRCS
	tests/PathTracerTest.cpp
)

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_files(tests-${LIB} ${TEST_FILES})
gtest_suite_deps(tests-${LIB} ${LIB} test-app)
gtest_suite_end(tests-${LIB})
