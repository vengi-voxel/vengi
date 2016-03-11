fips_begin_module(cooldown)
	fips_files(
		CooldownMgr.h CooldownMgr.cpp
		CooldownType.h
		CooldownTriggerState.h
	)
	fips_deps(core)
fips_end_module()

gtest_begin(cooldown)
	fips_dir(tests)
	fips_files(
		CooldownMgrTest.cpp
	)
	fips_deps(cooldown)
gtest_end()
