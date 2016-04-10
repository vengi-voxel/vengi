fips_begin_module(voxel)
	fips_files(
		BiomManager.h
		World.cpp World.h
		WorldEvents.h
		WorldData.h
		Voxel.h
	)
	fips_deps(core io noise zlib)
fips_end_module()

gtest_suite_files(tests tests/WorldTest.cpp)
gtest_suite_deps(tests voxel)
