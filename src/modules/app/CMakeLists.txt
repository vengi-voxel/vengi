set(SRCS
	App.cpp App.h
	AppCommand.cpp AppCommand.h
	CommandlineApp.h CommandlineApp.cpp
)

set(LIB app)

if (WINDOWS)
	# All this windows.h insanities ... giving up on this module
	set_property(GLOBAL PROPERTY ${LIB}_NOUNITY TRUE)
endif()

set(LIBS metric sdl2 glm libuv backward)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES ${LIBS})

set(TEST_MODULE_SRCS
	tests/AbstractTest.cpp
	tests/AbstractTest.h
)
engine_add_module(TARGET test-${LIB} SRCS ${TEST_MODULE_SRCS} DEPENDENCIES gtest ${LIB})

set(TEST_FILES
	testapp/iotest.txt
)

set(TEST_SRCS
	tests/AppTest.cpp
	tests/CommandCompleterTest.cpp
	tests/FilesystemTest.cpp
	tests/FileStreamTest.cpp
	tests/FileTest.cpp
)

gtest_suite_sources(tests ${TEST_SRCS})
gtest_suite_deps(tests ${LIB} test-app)
gtest_suite_files(tests ${TEST_FILES})

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_deps(tests-${LIB} ${LIB} test-app)
gtest_suite_files(tests-${LIB} ${TEST_FILES})
gtest_suite_end(tests-${LIB})

set(BENCHMARK_SRCS
	benchmark/AbstractBenchmark.cpp
)
engine_add_module(TARGET benchmark-${LIB} SRCS ${BENCHMARK_SRCS} DEPENDENCIES benchmark ${LIB})
