set(SRCS
	LUA.h LUA.cpp
	LUAFunctions.h LUAFunctions.cpp
	Trace.h
)
set(LIB commonlua)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES io lua54)

set(TEST_SRCS
	tests/LUAFunctionsTest.cpp
)

gtest_suite_sources(tests ${TEST_SRCS})
gtest_suite_deps(tests ${LIB} test-app)

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_deps(tests-${LIB} ${LIB} test-app)
gtest_suite_end(tests-${LIB})
