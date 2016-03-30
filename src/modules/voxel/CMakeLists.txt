fips_begin_module(voxel)
	fips_files(
		BiomManager.h
		Raycast.cpp Raycast.h
		World.cpp World.h
		WorldEvents.h
		WorldData.h
		Voxel.h
	)
	find_package(SDL2 REQUIRED)
	include_directories(${SDL2_INCLUDE_DIRS})
	fips_deps(core io noise zlib ${SDL2_LIBRARIES})
fips_end_module()

gtest_suite_files(tests tests/WorldTest.cpp)
gtest_suite_deps(tests voxel)
