fips_begin_module(commonlua)
	fips_files(
		LUA.h LUA.cpp
	)
	fips_deps(core lua52)
fips_end_module()

begin_googleunittest(commonlua)
	fips_dir(tests)
	fips_files(
		LUATest.cpp
	)
	fips_deps(commonlua)
end_googleunittest()
