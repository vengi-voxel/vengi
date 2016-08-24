set(SRCS
	IProgressMonitor.h
	PosLerp.h
	EMailValidator.h
	KeybindingParser.h
	Dummy.cpp
)
set(LIB util)
add_library(${LIB} ${SRCS})
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})

gtest_suite_files(tests
	tests/KeybindingParserTest.cpp
	tests/EMailValidatorTest.cpp
)
gtest_suite_deps(tests ${LIB})
