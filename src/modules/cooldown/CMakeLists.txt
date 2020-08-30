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

set(TEST_SRCS
	tests/CooldownProviderTest.cpp
	tests/CooldownMgrTest.cpp
)
gtest_suite_sources(tests ${TEST_SRCS})
gtest_suite_deps(tests ${LIB} test-app)

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_deps(tests-${LIB} ${LIB} test-app image)
gtest_suite_end(tests-${LIB})
