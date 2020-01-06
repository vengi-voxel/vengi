set(SRCS
	IProgressMonitor.h
	EMailValidator.h
	Console.h Console.cpp
	KeybindingParser.h KeybindingParser.cpp
	KeybindingHandler.h KeybindingHandler.cpp
	IncludeUtil.h IncludeUtil.cpp
	MessageQueue.h MessageQueue.cpp
)
set(LIB util)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES math)

set(TEST_SRCS
	tests/KeybindingParserTest.cpp
	tests/KeybindingHandlerTest.cpp
	tests/EMailValidatorTest.cpp
	tests/IncludeUtilTest.cpp
)
set(TEST_FILES
	testutil/main.h
	testutil/one.h
	testutil/two.h
)

gtest_suite_files(tests ${TEST_FILES})
gtest_suite_sources(tests ${TEST_SRCS})
gtest_suite_deps(tests ${LIB})

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_files(tests-${LIB} ${TEST_FILES})
gtest_suite_sources(tests-${LIB} ${TEST_SRCS} ../core/tests/AbstractTest.cpp)
gtest_suite_deps(tests-${LIB} ${LIB})
gtest_suite_end(tests-${LIB})
