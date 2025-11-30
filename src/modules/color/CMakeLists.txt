set(SRCS
	Color.cpp Color.h
	ColorUtil.cpp ColorUtil.h
	CMYK.cpp CMYK.h
	Quantize.cpp Quantize.h
	RGBA.h RGBA.cpp
)
set(LIB color)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES app)

set(TEST_SRCS
	tests/ColorTest.cpp
	tests/QuantizeTest.cpp
)

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_deps(tests-${LIB} ${LIB} core)
gtest_suite_end(tests-${LIB})

set(BENCHMARK_SRCS
	benchmarks/ColorBenchmark.cpp
)
engine_add_executable(TARGET benchmarks-${LIB} SRCS ${BENCHMARK_SRCS} NOINSTALL)
engine_target_link_libraries(TARGET benchmarks-${LIB} DEPENDENCIES benchmark-app ${LIB})
