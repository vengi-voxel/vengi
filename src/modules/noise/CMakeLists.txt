fips_begin_module(noise)
	fips_files(
		SimplexNoise.h SimplexNoise.cpp
	)
	fips_deps(core)
fips_end_module()

gtest_suite_files(tests tests/SimplexNoiseTest.cpp)
gtest_suite_deps(tests core image noise)
