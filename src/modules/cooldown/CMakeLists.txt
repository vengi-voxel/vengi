set(SRCS
	CooldownMgr.h CooldownMgr.cpp
	CooldownType.h
	Cooldown.h Cooldown.cpp
	CooldownProvider.h CooldownProvider.cpp
	CooldownTriggerState.h
)
set(LIB cooldown)
set(FILES
	shared/cooldowns.lua
)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} FILES ${FILES} DEPENDENCIES commonlua network)

gtest_suite_sources(tests
	tests/CooldownProviderTest.cpp
	tests/CooldownMgrTest.cpp
)
gtest_suite_deps(tests ${LIB})
