set(SRCS
	App.cpp App.h
	AppCommand.cpp AppCommand.h
	CommandlineApp.h CommandlineApp.cpp
)

set(LIB app)

set(LIBS metric command)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES ${LIBS})
if (WINDOWS)
	# All this windows.h insanities ... giving up on this module
	set_target_properties(${LIB} PROPERTIES UNITY_BUILD FALSE)
endif()

set(TEST_MODULE_SRCS
	tests/AbstractTest.cpp
	tests/AbstractTest.h
)
engine_add_module(TARGET test-${LIB} SRCS ${TEST_MODULE_SRCS} DEPENDENCIES gtest ${LIB})

set(TEST_SRCS
	tests/AppTest.cpp
	tests/CommandCompleterTest.cpp
)

gtest_suite_sources(tests ${TEST_SRCS})
gtest_suite_deps(tests ${LIB} test-app)

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_deps(tests-${LIB} ${LIB} test-app)
gtest_suite_end(tests-${LIB})

set(BENCHMARK_SRCS
	benchmark/AbstractBenchmark.cpp
)
engine_add_module(TARGET benchmark-${LIB} SRCS ${BENCHMARK_SRCS} DEPENDENCIES benchmark ${LIB})
