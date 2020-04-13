set(LIB voxelworld)
set(SRCS
	BiomeLUAFunctions.h BiomeLUAFunctions.cpp
	Biome.h Biome.cpp
	BiomeManager.h BiomeManager.cpp
	WorldMgr.cpp WorldMgr.h
	ChunkPersister.h ChunkPersister.cpp
	FilePersister.h FilePersister.cpp
	TreeVolumeCache.h TreeVolumeCache.cpp
	WorldPager.h WorldPager.cpp
	WorldEvents.h
	WorldContext.h WorldContext.cpp
)

set(FILES
	voxel/models/trees/bush/1.vox
	voxel/models/trees/deciduous/1.vox
	voxel/models/trees/deciduous/2.vox
	voxel/models/trees/deciduous/3.vox
	voxel/models/trees/fir/1.vox
	voxel/models/trees/palm/1.vox
	voxel/models/trees/pine/1.vox
	voxel/models/trees/trunk.vox
)

engine_add_module(TARGET ${LIB} SRCS ${SRCS} FILES ${FILES} DEPENDENCIES voxelformat noise)

set(TEST_SRCS
	tests/AbstractVoxelTest.h
	tests/FilePersisterTest.cpp
	tests/BiomeManagerTest.cpp
)

set(TEST_FILES
	shared/worldparams.lua
	shared/biomes.lua
)

gtest_suite_sources(tests ${TEST_SRCS})
gtest_suite_deps(tests ${LIB})
gtest_suite_files(tests ${TEST_FILES})

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS} ../core/tests/AbstractTest.cpp)
gtest_suite_files(tests-${LIB} ${TEST_FILES})
gtest_suite_deps(tests-${LIB} ${LIB})
gtest_suite_end(tests-${LIB})

set(BENCHMARK_SRCS
	../core/benchmark/AbstractBenchmark.cpp
	benchmarks/VoxelBenchmark.cpp
)
engine_add_executable(TARGET benchmarks-${LIB} SRCS ${BENCHMARK_SRCS} FILES ${FILES} shared/worldparams.lua shared/biomes.lua NOINSTALL)
engine_target_link_libraries(TARGET benchmarks-${LIB} DEPENDENCIES benchmark ${LIB})
