set(LIB voxelutil)
set(SRCS
	AStarPathfinder.h
	AStarPathfinderImpl.h
	Raycast.h
	Picking.h
	VolumeMerger.h VolumeMerger.cpp
	VolumeMover.h
	VolumeRescaler.h
	VolumeRotator.h VolumeRotator.cpp
	VolumeCropper.h
	VolumeVisitor.h
	RawVolumeRotateWrapper.h RawVolumeRotateWrapper.cpp
)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES voxel)

set(TEST_SRCS
	tests/PickingTest.cpp
	tests/VolumeMergerTest.cpp
	tests/VolumeRotatorTest.cpp
	tests/VolumeCropperTest.cpp
)

gtest_suite_sources(tests ${TEST_SRCS})
gtest_suite_deps(tests ${LIB})

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS} ../core/tests/AbstractTest.cpp)
gtest_suite_deps(tests-${LIB} ${LIB})
gtest_suite_end(tests-${LIB})
