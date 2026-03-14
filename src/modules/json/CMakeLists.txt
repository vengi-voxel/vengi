set(SRCS
	JSON.h JSON.cpp
)

set(LIB json)
set(LIBS core cjson)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES ${LIBS})

set(TEST_SRCS
	tests/JsonTest.cpp
)
gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_deps(tests-${LIB} ${LIB})
gtest_suite_end(tests-${LIB})

if (USE_BENCHMARKS)
	set(BENCHMARK_SRCS
		benchmarks/JsonBenchmark.cpp
	)
	engine_add_executable(TARGET benchmarks-${LIB} SRCS ${BENCHMARK_SRCS} NOINSTALL)
	engine_target_link_libraries(TARGET benchmarks-${LIB} DEPENDENCIES benchmark-app ${LIB})
endif()
