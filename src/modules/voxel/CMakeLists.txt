set(SRCS
	font/stb_truetype.h
	model/VoxFileFormat.h model/VoxFileFormat.cpp
	model/VoxFormat.h model/VoxFormat.cpp
	model/QBTFormat.h model/QBTFormat.cpp
	model/QBFormat.h model/QBFormat.cpp
	model/MeshExporter.h model/MeshExporter.cpp
	font/VoxelFont.h font/VoxelFont.cpp
	BiomeLUAFunctions.h BiomeLUAFunctions.cpp
	Biome.h Biome.cpp
	BiomeManager.h BiomeManager.cpp
	Spiral.h
	TreeContext.h TreeContext.cpp
	Constants.h
	MaterialColor.h MaterialColor.cpp
	RandomVoxel.h
	World.cpp World.h
	WorldPersister.h WorldPersister.cpp
	WorldPager.h WorldPager.cpp
	WorldEvents.h
	WorldContext.h WorldContext.cpp
	generator/CloudGenerator.h
	generator/ShapeGenerator.h
	generator/TreeGenerator.h generator/TreeGenerator.cpp
	generator/BuildingGenerator.h
	generator/WorldGenerator.h generator/WorldGenerator.cpp
	generator/LSystemGenerator.h
	generator/NoiseGenerator.h
	generator/CactusGenerator.h
	generator/PlantGenerator.h generator/PlantGenerator.cpp
	generator/PlanetGenerator.h
	polyvox/AStarPathfinder.h
	polyvox/AStarPathfinderImpl.h
	polyvox/CubicSurfaceExtractor.h polyvox/CubicSurfaceExtractor.cpp
	polyvox/Mesh.h polyvox/Mesh.cpp
	polyvox/Morton.h
	polyvox/PagedVolume.h polyvox/PagedVolume.cpp
	polyvox/PagedVolumeWrapper.h polyvox/PagedVolumeWrapper.cpp
	polyvox/RawVolume.h polyvox/RawVolume.cpp
	polyvox/RawVolumeWrapper.h
	polyvox/RawVolumeMoveWrapper.h
	polyvox/Raycast.h
	polyvox/Picking.h
	polyvox/Region.h polyvox/Region.cpp
	polyvox/Utility.h
	polyvox/VoxelVertex.h
	polyvox/Voxel.h polyvox/Voxel.cpp
	polyvox/VolumeMerger.h
	polyvox/VolumeMover.h
	polyvox/VolumeRescaler.h
	polyvox/VolumeRotator.h polyvox/VolumeRotator.cpp
	polyvox/VolumeCropper.h
	SurfaceExtractionTask.h SurfaceExtractionTask.cpp
	OctreeNode.h OctreeNode.cpp
	OctreeVolume.h OctreeVolume.cpp
	Octree.h Octree.cpp
)
set(LIB voxel)
add_library(${LIB} ${SRCS})
engine_target_link_libraries(TARGET ${LIB} DEPENDENCIES core noise zlib image assimp commonlua)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})
#target_compile_options(${LIB} PRIVATE -march=native PRIVATE -O3)

set(TEST_SRCS
	tests/AbstractVoxelTest.h
	tests/AbstractVoxFormatTest.h tests/AbstractVoxFormatTest.cpp
	tests/WorldTest.cpp
	tests/WorldPersisterTest.cpp
	tests/LSystemGeneratorTest.cpp
	tests/PolyVoxTest.cpp
	tests/PickingTest.cpp
	tests/BiomeManagerTest.cpp
	tests/AmbientOcclusionTest.cpp
	tests/OctreeTest.cpp
	tests/VoxFormatTest.cpp
	tests/QBTFormatTest.cpp
	tests/QBFormatTest.cpp
	tests/MeshExporterTest.cpp
	tests/VolumeMergerTest.cpp
	tests/VolumeRotatorTest.cpp
	tests/VolumeCropperTest.cpp
)

gtest_suite_files(tests ${TEST_SRCS})
gtest_suite_deps(tests ${LIB})

gtest_suite_begin(tests-voxel TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_files(tests-voxel ${TEST_SRCS} ../core/tests/AbstractTest.cpp)
gtest_suite_deps(tests-voxel ${LIB})
gtest_suite_end(tests-voxel)
