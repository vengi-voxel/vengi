set(LIB voxelworld)
set(SRCS
	BiomeLUAFunctions.h BiomeLUAFunctions.cpp
	Biome.h Biome.cpp
	BiomeManager.h BiomeManager.cpp
	TreeContext.h TreeContext.cpp
	WorldMgr.cpp WorldMgr.h
	WorldPersister.h WorldPersister.cpp
	WorldPager.h WorldPager.cpp
	WorldEvents.h
)
set(FILES
	shared/palette-nippon.lua
	shared/palette-nippon.png
)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} FILES ${FILES} DEPENDENCIES voxel)

set(TEST_SRCS
	tests/AbstractVoxelTest.h
	tests/WorldMgrTest.cpp
	tests/WorldPersisterTest.cpp
	tests/BiomeManagerTest.cpp
)

gtest_suite_sources(tests ${TEST_SRCS})
gtest_suite_deps(tests ${LIB})
gtest_suite_files(tests shared/worldparams.lua shared/biomes.lua)

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS} ../core/tests/AbstractTest.cpp)
gtest_suite_files(tests-${LIB} shared/worldparams.lua shared/biomes.lua)
gtest_suite_deps(tests-${LIB} ${LIB})
gtest_suite_end(tests-${LIB})

set(BENCHMARK_SRCS
	../core/benchmark/AbstractBenchmark.cpp
	benchmarks/VoxelBenchmark.cpp
)
engine_add_executable(TARGET benchmarks-${LIB} SRCS ${BENCHMARK_SRCS} FILES ${FILES} shared/worldparams.lua shared/biomes.lua NOINSTALL)
engine_target_link_libraries(TARGET benchmarks-${LIB} DEPENDENCIES benchmark ${LIB})
