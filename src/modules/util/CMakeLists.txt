set(SRCS
	IProgressMonitor.h
	PosLerp.h
	EMailValidator.h
	KeybindingParser.h
	CommandHandler.h CommandHandler.cpp
)
set(LIB util)
add_library(${LIB} ${SRCS})
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})
target_link_libraries(${LIB} core)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})

gtest_suite_files(tests
	tests/KeybindingParserTest.cpp
	tests/EMailValidatorTest.cpp
)
gtest_suite_deps(tests ${LIB})
