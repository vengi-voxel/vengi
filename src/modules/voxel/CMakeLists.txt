set(SRCS
	font/stb_truetype.h
	VoxFileFormat.h VoxFileFormat.cpp
	VoxFormat.h VoxFormat.cpp
	QB2Format.h QB2Format.cpp
	font/VoxelFont.h font/VoxelFont.cpp
	BiomeManager.h BiomeManager.cpp
	LodCreator.h
	Spiral.h
	Constants.h
	MaterialColor.h MaterialColor.cpp
	World.cpp World.h
	WorldPersister.h WorldPersister.cpp
	WorldEvents.h
	generator/CloudGenerator.h generator/CloudGenerator.cpp
	generator/ShapeGenerator.h generator/ShapeGenerator.cpp
	generator/TreeGenerator.h generator/TreeGenerator.cpp
	generator/WorldGenerator.h generator/WorldGenerator.cpp
	generator/LSystemGenerator.h generator/LSystemGenerator.cpp
	generator/PlantGenerator.h generator/PlantGenerator.cpp
	generator/PlanetGenerator.h generator/PlanetGenerator.cpp
	generator/GeneratorContext.h
	polyvox/Array.h
	polyvox/AStarPathfinder.h
	polyvox/AStarPathfinderImpl.h
	polyvox/CubicSurfaceExtractor.h polyvox/CubicSurfaceExtractor.cpp
	polyvox/Mesh.h polyvox/Mesh.cpp
	polyvox/Morton.h
	polyvox/PagedVolume.h polyvox/PagedVolume.cpp
	polyvox/RawVolume.h polyvox/RawVolume.cpp
	polyvox/Raycast.h
	polyvox/Picking.h
	polyvox/Region.h polyvox/Region.cpp
	polyvox/Utility.h
	polyvox/Vertex.h
	polyvox/Voxel.h
	polyvox/VolumeMerger.h
)
set(LIB voxel)
add_library(${LIB} ${SRCS})
target_link_libraries(${LIB} core noise zlib)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})
target_compile_options(${LIB} PRIVATE -march=native PRIVATE -O3)

gtest_suite_files(tests
	tests/AbstractVoxelTest.h
	tests/WorldTest.cpp
	tests/WorldPersisterTest.cpp
	tests/LSystemGeneratorTest.cpp
	tests/PolyVoxTest.cpp
	tests/PickingTest.cpp
	tests/BiomeManagerTest.cpp
	tests/AmbientOcclusionTest.cpp
	tests/VoxFormatTest.cpp
)
gtest_suite_deps(tests ${LIB})
