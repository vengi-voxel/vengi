fips_begin_module(cooldown)
	fips_files(
		CooldownMgr.h CooldownMgr.cpp
		CooldownType.h
		CooldownTriggerState.h
	)
	fips_deps(core)
fips_end_module()

gtest_suite_files(tests tests/CooldownMgrTest.cpp)
gtest_suite_deps(tests cooldown)
