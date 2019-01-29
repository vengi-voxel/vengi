project(computeshadertool)

set(SHARED_SRCS
	Types.h
	Parser.cpp Parser.h
	Generator.cpp Generator.h
	Util.cpp Util.h
)

set(SRCS
	${SHARED_SRCS}
	ComputeShaderTool.h ComputeShaderTool.cpp
)
engine_add_build_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} NOINSTALL)
if(NOT CMAKE_CROSSCOMPILING)
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES util compute simplecpp)

set(TEST_SRCS
	${SHARED_SRCS}
	tests/ComputeShaderToolTest.cpp
)

gtest_suite_begin(tests-${PROJECT_NAME} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${PROJECT_NAME} ${TEST_SRCS} ../../modules/core/tests/AbstractTest.cpp)
gtest_suite_deps(tests-${PROJECT_NAME} compute simplecpp)
gtest_suite_end(tests-${PROJECT_NAME})

gtest_suite_sources(tests ${TEST_SRCS})
gtest_suite_deps(tests compute simplecpp)
endif(NOT CMAKE_CROSSCOMPILING)
