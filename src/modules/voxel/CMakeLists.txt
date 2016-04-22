fips_begin_module(voxel)
	fips_files(
		BiomManager.h
		LodCreator.h
		Spiral.h
		Voxel.h
		World.cpp World.h
		WorldData.h
		WorldEvents.h
	)
	fips_dir(polyvox)
	fips_files(
		Array.h
		AStarPathfinder.h
		AStarPathfinderImpl.h
		BaseVolume.h
		CubicSurfaceExtractor.h
		DefaultIsQuadNeeded.h
		Mesh.h
		Morton.h
		PagedVolume.h PagedVolume.cpp
		RawVolume.h
		Raycast.h
		Region.h
		Utility.h
		Vertex.h
		Voxel.h
	)
	fips_deps(core io noise zlib)
fips_end_module()

gtest_suite_files(tests tests/WorldTest.cpp)
gtest_suite_deps(tests voxel)
