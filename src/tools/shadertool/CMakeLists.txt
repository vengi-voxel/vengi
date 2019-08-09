project(shadertool)

set(SHARED_SRCS
	Generator.cpp Generator.h
	Types.cpp Types.h
	Util.cpp Util.h
	TokenIterator.h
	Parser.cpp Parser.h
)

set(SRCS
	ShaderTool.h ShaderTool.cpp
	${SHARED_SRCS}
)
if (MSVC)
	set_property(GLOBAL PROPERTY ${PROJECT_NAME}_NOUNITY TRUE)
endif()
engine_add_build_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} NOINSTALL)
if(NOT CMAKE_CROSSCOMPILING)
add_dependencies(${PROJECT_NAME} glslangValidator)
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES video util simplecpp)

set(TEST_SRCS
	${SHARED_SRCS}
	tests/ShaderToolTest.cpp
)

gtest_suite_begin(tests-${PROJECT_NAME} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${PROJECT_NAME} ${TEST_SRCS} ../../modules/core/tests/AbstractTest.cpp)
gtest_suite_deps(tests-${PROJECT_NAME} video simplecpp)
gtest_suite_end(tests-${PROJECT_NAME})

gtest_suite_sources(tests ${TEST_SRCS})
gtest_suite_deps(tests video simplecpp)
endif(NOT CMAKE_CROSSCOMPILING)
