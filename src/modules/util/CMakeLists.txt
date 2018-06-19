set(SRCS
	IProgressMonitor.h
	PosLerp.h
	EMailValidator.h
	Console.h Console.cpp
	KeybindingParser.h KeybindingParser.cpp
	KeybindingHandler.h KeybindingHandler.cpp
	IncludeUtil.h IncludeUtil.cpp
)
set(LIB util)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES math)

gtest_suite_sources(tests
	tests/KeybindingParserTest.cpp
	tests/KeybindingHandlerTest.cpp
	tests/EMailValidatorTest.cpp
)
gtest_suite_deps(tests ${LIB})
