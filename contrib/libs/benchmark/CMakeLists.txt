set(SRCS
	src/benchmark_api_internal.cc
	src/benchmark.cc
	src/benchmark_main.cc
	src/benchmark_name.cc
	src/benchmark_register.cc
	src/benchmark_runner.cc
	src/check.cc
	src/colorprint.cc
	src/commandlineflags.cc
	src/complexity.cc
	src/console_reporter.cc
	src/counter.cc
	src/csv_reporter.cc
	src/json_reporter.cc
	src/perf_counters.cc
	src/reporter.cc
	src/statistics.cc
	src/string_util.cc
	src/sysinfo.cc
	src/timers.cc
)


set(LIB benchmark)
engine_add_library(
	LIB ${LIB}
	SRCS ${SRCS}
	DEFINES HAVE_STD_REGEX=1 BENCHMARK_STATIC_DEFINE
)
target_include_directories(${LIB} PUBLIC include)
if (WIN32)
	target_link_libraries(${LIB} shlwapi)
endif()
