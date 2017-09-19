set(SRCS
	EventMgr.h EventMgr.cpp
)
set(LIB eventmgr)
add_library(${LIB} ${SRCS})
engine_target_link_libraries(TARGET ${LIB} DEPENDENCIES core commonlua network)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})

gtest_suite_files(tests
	tests/EventMgrTest.cpp
)
gtest_suite_deps(tests ${LIB})
