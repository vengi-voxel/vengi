set(LIB voxel)
set(SRCS
	Constants.h
	RandomVoxel.h RandomVoxel.cpp
	CubicSurfaceExtractor.h CubicSurfaceExtractor.cpp
	Face.h Face.cpp
	MaterialColor.h MaterialColor.cpp
	Mesh.h Mesh.cpp
	Morton.h
	Palette.h Palette.cpp
	PagedVolume.h PagedVolume.cpp
	PagedVolumeSampler.cpp PagedVolumeChunk.cpp
	PagedVolumeWrapper.h PagedVolumeWrapper.cpp
	RawVolume.h RawVolume.cpp
	RawVolumeWrapper.h
	RawVolumeMoveWrapper.h
	Region.h Region.cpp
	VoxelVertex.h
	Voxel.h Voxel.cpp
)
set(FILES
	shared/palette-nippon.lua
	shared/palette-nippon.png
)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} FILES ${FILES} DEPENDENCIES util image commonlua)
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
	tests/PaletteTest.cpp
	tests/PolyVoxTest.cpp
	tests/RegionTest.cpp
	tests/TestHelper.h
	tests/AmbientOcclusionTest.cpp
	tests/RawVolumeWrapperTest.cpp
)

gtest_suite_sources(tests ${TEST_SRCS})
gtest_suite_deps(tests ${LIB} test-app)
gtest_suite_files(tests ${FILES})

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_deps(tests-${LIB} ${LIB} test-app)
gtest_suite_files(tests-${LIB} ${FILES})
gtest_suite_end(tests-${LIB})

set(BENCHMARK_SRCS
	benchmarks/CubicSurfaceExtractorBenchmark.cpp
)
engine_add_executable(TARGET benchmarks-${LIB} SRCS ${BENCHMARK_SRCS} NOINSTALL)
engine_target_link_libraries(TARGET benchmarks-${LIB} DEPENDENCIES benchmark-app ${LIB})
