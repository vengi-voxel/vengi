set(LIB voxel)
set(SRCS
	private/CubicSurfaceExtractor.h private/CubicSurfaceExtractor.cpp
	private/MarchingCubesSurfaceExtractor.h private/MarchingCubesSurfaceExtractor.cpp
	private/MarchingCubesTables.h

	SurfaceExtractor.h SurfaceExtractor.cpp
	ChunkMesh.h
	Face.h Face.cpp
	MaterialColor.h MaterialColor.cpp
	Mesh.h Mesh.cpp
	MeshState.h MeshState.cpp
	RawVolume.h RawVolume.cpp
	RawVolumeWrapper.h
	RawVolumeMoveWrapper.h
	Region.h Region.cpp
	SparseVolume.h SparseVolume.cpp
	VoxelVertex.h
	Voxel.h Voxel.cpp
)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES commonlua palette meshoptimizer)
engine_target_optimize(${LIB})

set(TEST_SRCS
	tests/AbstractVoxelTest.h
	tests/AmbientOcclusionTest.cpp
	tests/FaceTest.cpp
	tests/MeshTests.cpp
	tests/MeshStateTest.cpp
	tests/MortonTest.cpp
	tests/RawVolumeTest.cpp
	tests/RegionTest.cpp
	tests/SparseVolumeTest.cpp
	tests/SurfaceExtractorTest.cpp
	tests/RawVolumeWrapperTest.cpp
)

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_deps(tests-${LIB} ${LIB} test-app)
gtest_suite_end(tests-${LIB})
