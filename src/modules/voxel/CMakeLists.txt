set(SRCS
	font/stb_truetype.h
	model/VoxFileFormat.h model/VoxFileFormat.cpp
	model/VoxFormat.h model/VoxFormat.cpp
	model/QB2Format.h model/QB2Format.cpp
	model/MeshExporter.h model/MeshExporter.cpp
	font/VoxelFont.h font/VoxelFont.cpp
	BiomeManager.h BiomeManager.cpp
	LodCreator.h
	Spiral.h
	TreeContext.h
	Constants.h
	MaterialColor.h MaterialColor.cpp
	World.cpp World.h
	WorldPersister.h WorldPersister.cpp
	WorldEvents.h
	generator/CloudGenerator.h
	generator/ShapeGenerator.h generator/ShapeGenerator.cpp
	generator/TreeGenerator.h
	generator/WorldGenerator.h
	generator/LSystemGenerator.h
	generator/CactusGenerator.h
	generator/PlantGenerator.h generator/PlantGenerator.cpp
	generator/PlanetGenerator.h
	polyvox/Array.h
	polyvox/AStarPathfinder.h
	polyvox/AStarPathfinderImpl.h
	polyvox/CubicSurfaceExtractor.h polyvox/CubicSurfaceExtractor.cpp
	polyvox/Mesh.h polyvox/Mesh.cpp
	polyvox/Morton.h
	polyvox/PagedVolume.h polyvox/PagedVolume.cpp
	polyvox/PagedVolumeWrapper.h
	polyvox/RawVolume.h polyvox/RawVolume.cpp
	polyvox/RawVolumeWrapper.h
	polyvox/RawVolumeMoveWrapper.h
	polyvox/Raycast.h
	polyvox/Picking.h
	polyvox/Region.h polyvox/Region.cpp
	polyvox/Utility.h
	polyvox/Vertex.h
	polyvox/Voxel.h
	polyvox/VolumeMerger.h
	polyvox/VolumeMover.h
	polyvox/VolumeRotator.h polyvox/VolumeRotator.cpp
	polyvox/VolumeCropper.h polyvox/VolumeCropper.cpp
)
set(LIB voxel)
add_library(${LIB} ${SRCS})
target_link_libraries(${LIB} core noise zlib assimp)
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
	tests/QB2FormatTest.cpp
	tests/MeshExporterTest.cpp
	tests/VolumeMergerTest.cpp
	tests/VolumeRotatorTest.cpp
	tests/VolumeCropperTest.cpp
)
gtest_suite_deps(tests ${LIB})
