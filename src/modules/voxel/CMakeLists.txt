fips_begin_module(voxel)
	fips_files(
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

begin_googleunittest(voxel)
	fips_dir(tests)
	fips_files(
		WorldTest.cpp
	)
	find_package(SDL2 REQUIRED)
	include_directories(${SDL2_INCLUDE_DIRS})
	fips_deps(voxel ${SDL2_LIBRARIES})
end_googleunittest()
