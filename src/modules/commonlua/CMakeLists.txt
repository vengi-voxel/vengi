fips_begin_module(commonlua)
	fips_files(
		LUA.h LUA.cpp
	)
	fips_deps(core lua52)
fips_end_module()

gtest_begin(commonlua)
	fips_dir(tests)
	fips_files(
		LUATest.cpp
	)
	fips_deps(commonlua)
gtest_end()
