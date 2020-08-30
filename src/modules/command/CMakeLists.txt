set(SRCS
	ActionButton.h ActionButton.cpp
	CommandHandler.h CommandHandler.cpp
	CommandCompleter.h CommandCompleter.cpp
	Command.h Command.cpp
)
set(LIB command)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES io)

set(TEST_SRCS
	tests/CommandTest.cpp
	tests/CommandHandlerTest.cpp
)

gtest_suite_sources(tests ${TEST_SRCS})
gtest_suite_deps(tests ${LIB})

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_deps(tests-${LIB} ${LIB})
gtest_suite_end(tests-${LIB})
