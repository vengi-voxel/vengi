set(SRCS
	LUA.h LUA.cpp
	LUAFunctions.h LUAFunctions.cpp
)
set(LIB commonlua)
add_library(${LIB} ${SRCS})
engine_target_link_libraries(TARGET ${LIB} DEPENDENCIES core lua53)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})

gtest_suite_files(tests
	tests/LUAFunctionsTest.cpp
)
gtest_suite_deps(tests ${LIB})
