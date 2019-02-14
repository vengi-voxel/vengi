set(SRCS
	src/arraysize.h
	src/benchmark_api_internal.h
	src/benchmark.cc
	src/benchmark_register.cc
	src/check.h
	src/colorprint.cc
	src/colorprint.h
	src/commandlineflags.cc
	src/commandlineflags.h
	src/complexity.cc
	src/complexity.h
	src/console_reporter.cc
	src/counter.cc
	src/counter.h
	src/csv_reporter.cc
	src/cycleclock.h
	src/internal_macros.h
	src/json_reporter.cc
	src/log.h
	src/mutex.h
	src/re.h
	src/reporter.cc
	src/sleep.cc
	src/sleep.h
	src/statistics.cc
	src/statistics.h
	src/stat.h
	src/string_util.cc
	src/string_util.h
	src/sysinfo.cc
	src/sysinfo.h
	src/timers.cc
	src/timers.h
)

engine_add_library(
	LIB benchmark
	SRCS ${SRCS}
	DEFINES HAVE_STD_REGEX=1
)
target_include_directories(benchmark PUBLIC include)
