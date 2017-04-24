set(SRCS
	CooldownMgr.h CooldownMgr.cpp
	CooldownType.h
	CooldownProvider.h CooldownProvider.cpp
	CooldownTriggerState.h
)
set(LIB cooldown)
add_library(${LIB} ${SRCS})
engine_target_link_libraries(TARGET ${LIB} DEPENDENCIES core commonlua network)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})

gtest_suite_files(tests
	tests/CooldownProviderTest.cpp
	tests/CooldownMgrTest.cpp
)
gtest_suite_deps(tests ${LIB})
