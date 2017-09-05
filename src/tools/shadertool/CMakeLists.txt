project(shadertool)
set(SRCS
	ShaderTool.h ShaderTool.cpp
	Util.h Util.cpp
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS})
add_dependencies(${PROJECT_NAME} glslangValidator)
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES video simplecpp)

set(TEST_SRCS
	Util.cpp Util.h
	tests/ShaderToolTest.cpp
)

gtest_suite_begin(tests-${PROJECT_NAME} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_files(tests-${PROJECT_NAME} ${TEST_SRCS} ../../modules/core/tests/AbstractTest.cpp)
gtest_suite_deps(tests-${PROJECT_NAME} video simplecpp)
gtest_suite_end(tests-${PROJECT_NAME})

gtest_suite_files(tests ${TEST_SRCS})
gtest_suite_deps(tests video simplecpp)
