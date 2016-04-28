set(SRCS
	LUA.h LUA.cpp
)
set(LIB commonlua)
add_library(${LIB} ${SRCS})
target_link_libraries(${LIB} core lua53)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})

gtest_suite_files(tests
	tests/LUATest.cpp
)
gtest_suite_deps(tests ${LIB})
