set(SRCS
	EMailValidator.h EMailValidator.cpp
	Base64.cpp Base64.h
	BufferUtil.cpp BufferUtil.h
	Console.h Console.cpp
	Easing.h
	KeybindingParser.h KeybindingParser.cpp
	KeybindingHandler.h KeybindingHandler.cpp
	IncludeUtil.h IncludeUtil.cpp
	Movement.cpp Movement.h
	VarUtil.h
)
set(LIB util)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES math command)

set(TEST_SRCS
	tests/Base64Test.cpp
	tests/BufferUtilTest.cpp
	tests/ConsoleTest.cpp
	tests/EMailValidatorTest.cpp
	tests/IncludeUtilTest.cpp
	tests/KeybindingParserTest.cpp
	tests/KeybindingHandlerTest.cpp
)
set(TEST_FILES
	testutil/main.h
	testutil/one.h
	testutil/two.h
)

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_files(tests-${LIB} ${TEST_FILES})
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_deps(tests-${LIB} ${LIB} test-app)
gtest_suite_end(tests-${LIB})
