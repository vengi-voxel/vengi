set(LIB voxelpathtracer)
set(SRCS
	PathTracer.cpp PathTracer.h
)

engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES yocto scenegraph image)

set(TEST_SRCS
	tests/PathTracerTest.cpp
)
set(TEST_FILES
	tests/hmec.vxl
)
gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_files(tests-${LIB} ${TEST_FILES})
gtest_suite_deps(tests-${LIB} ${LIB} test-app voxelformat)
gtest_suite_end(tests-${LIB})
