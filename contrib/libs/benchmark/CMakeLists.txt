set(SRCS
	src/arraysize.h
	src/benchmark_api_internal.h
	src/benchmark.cc
	src/benchmark_register.cc
	src/benchmark_api_internal.cc
	src/benchmark_name.cc
	src/benchmark_runner.h
	src/benchmark_runner.cc
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


set(LIB benchmark)
engine_add_library(
	LIB ${LIB}
	SRCS ${SRCS}
	DEFINES HAVE_STD_REGEX=1
)
target_include_directories(${LIB} PUBLIC include)

if (WIN32)
	target_link_libraries(${LIB} shlwapi)
endif()
