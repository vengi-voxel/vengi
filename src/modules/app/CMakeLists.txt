set(SRCS
	App.cpp App.h
	AppCommand.cpp AppCommand.h
	Async.cpp Async.h
	CommandlineApp.h CommandlineApp.cpp

	i18n/Dictionary.cpp i18n/Dictionary.h
	i18n/DictionaryManager.cpp i18n/DictionaryManager.h
	i18n/Iconv.cpp i18n/Iconv.h
	i18n/Language.cpp i18n/Language.h
	i18n/PluralForms.cpp i18n/PluralForms.h
	i18n/POParser.cpp i18n/POParser.h
)
if (APPLE)
	list(APPEND SRCS system/AppDocument.m)
endif()
if (WINDOWS)
	list(APPEND SRCS system/Windows.cpp system/Windows.h)
endif()
set(LIB app)

set(LIBS command metric)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES ${LIBS})
if (WINDOWS)
	# All this windows.h insanities ... giving up on this module
	set_target_properties(${LIB} PROPERTIES UNITY_BUILD FALSE)
endif()
if (APPLE)
	target_link_libraries(${LIB} PRIVATE "-framework CoreFoundation -framework AppKit")
endif()

if (USE_STACKTRACES)
	target_compile_definitions(${LIB} PRIVATE HAVE_BACKWARD)
endif()

set(TEST_MODULE_SRCS
	tests/AbstractTest.cpp
	tests/AbstractTest.h
)
engine_add_module(TARGET test-${LIB} SRCS ${TEST_MODULE_SRCS} DEPENDENCIES gtest ${LIB})

set(TEST_SRCS
	tests/AppTest.cpp
	tests/AsyncTest.cpp
	tests/CommandCompleterTest.cpp
	tests/POParserTest.cpp
)

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_deps(tests-${LIB} test-app)
gtest_suite_end(tests-${LIB})

set(BENCHMARK_SRCS
	benchmark/AbstractBenchmark.cpp
)
engine_add_module(TARGET benchmark-${LIB} SRCS ${BENCHMARK_SRCS} DEPENDENCIES benchmark ${LIB})

set(BENCHMARK_APP_SRCS
	benchmarks/DictionaryBenchmark.cpp
)
engine_add_executable(TARGET benchmarks-${LIB} SRCS ${BENCHMARK_APP_SRCS} NOINSTALL)
engine_target_link_libraries(TARGET benchmarks-${LIB} DEPENDENCIES benchmark-app ${LIB})
