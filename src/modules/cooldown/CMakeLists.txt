set(SRCS
	CooldownMgr.h CooldownMgr.cpp
	CooldownType.h
	CooldownTriggerState.h
)
set(LIB cooldown)
add_library(${LIB} ${SRCS})
target_link_libraries(${LIB} core)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})

gtest_suite_files(tests
	tests/CooldownMgrTest.cpp
)
gtest_suite_deps(tests ${LIB})
