set(LIB voxelutil)
set(SRCS
	AStarPathfinder.h
	AStarPathfinderImpl.h
	ImageUtils.h ImageUtils.cpp
	Raycast.h
	Picking.h
	VolumeMerger.h VolumeMerger.cpp
	VolumeMover.h
	VolumeRescaler.h
	VolumeRotator.h VolumeRotator.cpp
	VolumeResizer.h VolumeResizer.cpp
	VolumeCropper.h
	VolumeSplitter.h VolumeSplitter.cpp
	VolumeVisitor.h
	VoxelUtil.h VoxelUtil.cpp
)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES voxel)

set(TEST_SRCS
	tests/AStarPathfinderTest.cpp
	tests/ImageUtilsTest.cpp
	tests/PickingTest.cpp
	tests/VolumeMergerTest.cpp
	tests/VolumeRotatorTest.cpp
	tests/VolumeSplitterTest.cpp
	tests/VolumeCropperTest.cpp
	tests/VolumeVisitorTest.cpp
	tests/VoxelUtilTest.cpp
)

set(TEST_FILES
	tests/test-palette-in.png
	tests/test-fillplane.png
	tests/test-heightmap.png
	tests/test-heightmap-dm.png
)

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_files(tests-${LIB} ${TEST_FILES})
gtest_suite_deps(tests-${LIB} ${LIB} test-app)
gtest_suite_end(tests-${LIB})

set(BENCHMARK_SRCS
	benchmarks/VoxelVisitorBenchmark.cpp
)
engine_add_executable(TARGET benchmarks-${LIB} SRCS ${BENCHMARK_SRCS} NOINSTALL)
engine_target_link_libraries(TARGET benchmarks-${LIB} DEPENDENCIES benchmark-app ${LIB})
