fips_begin_module(commonlua)
	fips_files(
		LUA.h LUA.cpp
	)
	fips_deps(core lua52)
fips_end_module()

gtest_suite_files(tests tests/LUATest.cpp)
gtest_suite_deps(tests commonlua)
