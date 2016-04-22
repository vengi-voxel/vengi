fips_begin_module(voxel)
	fips_files(
		BiomManager.h
		LodCreator.h
		Spiral.h
		Voxel.h
		World.cpp World.h
		WorldPersister.h WorldPersister.cpp
		WorldEvents.h
	)
	fips_dir(polyvox)
	fips_files(
		Array.h
		AStarPathfinder.h
		AStarPathfinderImpl.h
		BaseVolume.h BaseVolume.cpp
		CubicSurfaceExtractor.h
		Mesh.h Mesh.cpp
		Morton.h
		PagedVolume.h PagedVolume.cpp
		RawVolume.h RawVolume.cpp
		Raycast.h
		Region.h
		Utility.h
		Vertex.h
		Voxel.h
	)
	fips_deps(core io noise zlib)
fips_end_module()

gtest_suite_files(tests tests/AbstractVoxelTest.h tests/WorldTest.cpp tests/WorldPersisterTest.cpp)
gtest_suite_deps(tests voxel)
