set(LIB voxel)
set(SRCS
	Constants.h
	RandomVoxel.h
	AStarPathfinder.h
	AStarPathfinderImpl.h
	CubicSurfaceExtractor.h CubicSurfaceExtractor.cpp
	Face.h Face.cpp
	MaterialColor.h MaterialColor.cpp
	Mesh.h Mesh.cpp
	Morton.h
	PagedVolume.h PagedVolume.cpp
	PagedVolumeSampler.cpp PagedVolumeChunk.cpp
	PagedVolumeBufferedSampler.cpp
	PagedVolumeWrapper.h PagedVolumeWrapper.cpp
	RawVolume.h RawVolume.cpp
	RawVolumeWrapper.h
	RawVolumeMoveWrapper.h
	Raycast.h
	Picking.h
	Region.h Region.cpp
	Utility.h
	VoxelVertex.h
	Voxel.h Voxel.cpp
	VolumeMerger.h VolumeMerger.cpp
	VolumeMover.h
	VolumeRescaler.h
	VolumeRotator.h VolumeRotator.cpp
	VolumeCropper.h
	VolumeVisitor.h
)
set(FILES
	shared/palette-nippon.lua
	shared/palette-nippon.png
)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} FILES ${FILES} DEPENDENCIES noise image commonlua)
set(MARCH native)
#set(MARCH generic)
# http://christian-seiler.de/projekte/fpmath/
if (MSVC)
	# Errors out with RTC1 and Ox are incompatible
	#target_compile_options(${LIB} PRIVATE /Ox)
else()
	check_c_compiler_flag(-mtune=${MARCH} HAVE_FLAG_MTUNE_${MARCH})
	if (HAVE_FLAG_MTUNE_${MARCH})
		target_compile_options(${LIB} PRIVATE -mtune=${MARCH})
	endif()
	target_compile_options(${LIB} PRIVATE -O3)
endif()

set(TEST_SRCS
	tests/AbstractVoxelTest.h
	tests/FaceTest.cpp
	tests/PolyVoxTest.cpp
	tests/PickingTest.cpp
	tests/RegionTest.cpp
	tests/AmbientOcclusionTest.cpp
	tests/PagedVolumeBufferedSamplerTest.cpp
	tests/RawVolumeWrapperTest.cpp
	tests/VolumeMergerTest.cpp
	tests/VolumeRotatorTest.cpp
	tests/VolumeCropperTest.cpp
)

gtest_suite_sources(tests ${TEST_SRCS})
gtest_suite_deps(tests ${LIB})
gtest_suite_files(tests ${FILES})

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS} ../core/tests/AbstractTest.cpp)
gtest_suite_deps(tests-${LIB} ${LIB})
gtest_suite_files(tests-${LIB} ${FILES})
gtest_suite_end(tests-${LIB})
