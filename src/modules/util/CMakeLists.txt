set(SRCS
	BinaryPList.cpp BinaryPList.h
	BufferUtil.cpp BufferUtil.h
	Console.h Console.cpp
	KeybindingParser.h KeybindingParser.cpp
	KeybindingHandler.h KeybindingHandler.cpp
	IncludeUtil.h IncludeUtil.cpp
	IniParser.h IniParser.cpp
	Movement.cpp Movement.h
	TextProcessor.cpp
	VarUtil.h
	Version.h Version.cpp
	VersionCheck.h VersionCheck.cpp
)
set(LIB util)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES math command http json io)

set(TEST_SRCS
	tests/BinaryPListTest.cpp
	tests/BufferUtilTest.cpp
	tests/ConsoleTest.cpp
	tests/IncludeUtilTest.cpp
	tests/IniParserTest.cpp
	tests/KeybindingParserTest.cpp
	tests/KeybindingHandlerTest.cpp
	tests/TextProcessorTest.cpp
	tests/VersionCheckTest.cpp
	tests/VersionTest.cpp
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
