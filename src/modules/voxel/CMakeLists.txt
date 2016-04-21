fips_begin_module(voxel)
	fips_files(
		BiomManager.h
		World.cpp World.h
		WorldEvents.h
		WorldData.h
	)
	fips_dir(polyvox)
	fips_files(
		Array.h
		AStarPathfinder.h
		AStarPathfinderImpl.h
		BaseVolume.h
		CubicSurfaceExtractor.h
		DefaultIsQuadNeeded.h
		Interpolation.h
		IteratorController.h
		LowPassFilter.h
		MaterialDensityPair.h
		Material.h
		Mesh.h
		Morton.h
		PagedVolume.h
		Picking.h
		RandomUnitVectors.h
		RandomVectors.h
		RawVolume.h
		Raycast.h
		Region.h
		Timer.h
		Utility.h
		Vertex.h
		Voxel.h
		VolumeResampler.h
	)
	fips_deps(core io noise zlib)
fips_end_module()

gtest_suite_files(tests tests/WorldTest.cpp)
gtest_suite_deps(tests voxel)
