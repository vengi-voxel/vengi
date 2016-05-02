set(SRCS
	BiomeManager.h BiomeManager.cpp
	LodCreator.h
	Spiral.h
	Voxel.h
	World.cpp World.h
	WorldPersister.h WorldPersister.cpp
	WorldEvents.h
	generator/CloudGenerator.h generator/CloudGenerator.cpp
	generator/ShapeGenerator.h generator/ShapeGenerator.cpp
	generator/TreeGenerator.h generator/TreeGenerator.cpp
	generator/WorldGenerator.h generator/WorldGenerator.cpp
	generator/LSystemGenerator.h generator/LSystemGenerator.cpp
	polyvox/Array.h
	polyvox/AStarPathfinder.h
	polyvox/AStarPathfinderImpl.h
	polyvox/CubicSurfaceExtractor.h
	polyvox/Mesh.h polyvox/Mesh.cpp
	polyvox/Morton.h
	polyvox/PagedVolume.h polyvox/PagedVolume.cpp
	polyvox/RawVolume.h polyvox/RawVolume.cpp
	polyvox/Raycast.h
	polyvox/Region.h
	polyvox/Utility.h
	polyvox/Vertex.h
	polyvox/Voxel.h
)
set(LIB voxel)
add_library(${LIB} ${SRCS})
target_link_libraries(${LIB} core noise zlib)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})

gtest_suite_files(tests
	tests/AbstractVoxelTest.h
	tests/WorldTest.cpp
	tests/WorldPersisterTest.cpp
	tests/LSystemGeneratorTest.cpp
)
gtest_suite_deps(tests ${LIB})
