project(computeshadertool)
set(SRCS
	Util.cpp Util.h
	ComputeShaderTool.h ComputeShaderTool.cpp
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS})
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES compute simplecpp)

set(TEST_SRCS
	Util.cpp Util.h
	tests/ComputeShaderToolTest.cpp
)

gtest_suite_begin(tests-${PROJECT_NAME} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_files(tests-${PROJECT_NAME} ${TEST_SRCS} ../../modules/core/tests/AbstractTest.cpp)
gtest_suite_deps(tests-${PROJECT_NAME} compute simplecpp)
gtest_suite_end(tests-${PROJECT_NAME})

gtest_suite_files(tests ${TEST_SRCS})
gtest_suite_deps(tests compute simplecpp)
