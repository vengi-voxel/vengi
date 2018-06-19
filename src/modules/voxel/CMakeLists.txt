set(LIB voxel)
set(SRCS
	BiomeLUAFunctions.h BiomeLUAFunctions.cpp
	Biome.h Biome.cpp
	BiomeManager.h BiomeManager.cpp
	Spiral.h
	TreeContext.h TreeContext.cpp
	Constants.h
	MaterialColor.h MaterialColor.cpp
	RandomVoxel.h
	WorldMgr.cpp WorldMgr.h
	WorldPersister.h WorldPersister.cpp
	WorldPager.h WorldPager.cpp
	WorldEvents.h
	generator/CloudGenerator.h
	generator/ShapeGenerator.h
	generator/SpaceColonization.h generator/SpaceColonization.cpp
	generator/TreeGenerator.h generator/TreeGenerator.cpp
	generator/BuildingGenerator.h
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
	polyvox/PagedVolumeSampler.cpp polyvox/PagedVolumeChunk.cpp
	polyvox/PagedVolumeBufferedSampler.cpp
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
	polyvox/VolumeVisitor.h
)
set(FILES
	shared/palette-nippon.lua
	shared/palette-nippon.png
	shared/biomes.lua
)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} FILES ${FILES} DEPENDENCIES core math collection noise zlib image assimp commonlua)
set(MARCH native)
#set(MARCH generic)
# http://christian-seiler.de/projekte/fpmath/
if (MSVC)
	target_compile_options(${LIB} PRIVATE /Ox)
else()
	check_c_compiler_flag(-mtune=${MARCH} HAVE_FLAG_MTUNE_${MARCH})
	if (HAVE_FLAG_MTUNE_${MARCH})
		target_compile_options(${LIB} PRIVATE -mtune=${MARCH})
	endif()
	target_compile_options(${LIB} PRIVATE -O3)
endif()

set(TEST_SRCS
	tests/AbstractVoxelTest.h
	tests/WorldMgrTest.cpp
	tests/WorldPersisterTest.cpp
	tests/LSystemGeneratorTest.cpp
	tests/PolyVoxTest.cpp
	tests/PickingTest.cpp
	tests/BiomeManagerTest.cpp
	tests/AmbientOcclusionTest.cpp
	tests/PagedVolumeBufferedSamplerTest.cpp
	tests/VolumeMergerTest.cpp
	tests/VolumeRotatorTest.cpp
	tests/VolumeCropperTest.cpp
)

gtest_suite_sources(tests ${TEST_SRCS})
gtest_suite_deps(tests ${LIB})
gtest_suite_files(tests ${FILES})

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS} ../core/tests/AbstractTest.cpp)
gtest_suite_files(tests-${LIB} ${FILES} shared/worldparams.lua)
gtest_suite_deps(tests-${LIB} ${LIB})
gtest_suite_end(tests-${LIB})

set(BENCHMARK_SRCS
	../core/benchmark/AbstractBenchmark.cpp
	benchmarks/VoxelBenchmark.cpp
)
engine_add_executable(TARGET benchmarks-${LIB} SRCS ${BENCHMARK_SRCS} FILES ${FILES} shared/worldparams.lua NOINSTALL)
engine_target_link_libraries(TARGET benchmarks-${LIB} DEPENDENCIES benchmark ${LIB})
