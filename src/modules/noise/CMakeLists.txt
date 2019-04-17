set(SRCS
	Simplex.h
	Noise.h Noise.cpp
	PoissonDiskDistribution.h PoissonDiskDistribution.cpp

	shaders/noise.cl
)
# TODO: maybe provide two noise modules, one noisefast (for e.g. client only stuff) and one noise-slow for stuff that must be cross plattform

set(LIB noise)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES compute)
#set(MARCH native)
set(MARCH generic)
# http://christian-seiler.de/projekte/fpmath/
if (NOT MSVC)
	check_c_compiler_flag(-ffloat-store HAVE_FLAG_FLOAT_STORE)
	if (HAVE_FLAG_FLOAT_STORE)
		target_compile_options(${LIB} PRIVATE -ffloat-store)
	endif()
	check_c_compiler_flag(-mtune=${MARCH} HAVE_FLAG_MTUNE_${MARCH})
	if (HAVE_FLAG_MTUNE_${MARCH})
		target_compile_options(${LIB} PRIVATE -mtune=${MARCH})
	endif()
	target_compile_options(${LIB} PRIVATE -O3)
endif()
generate_compute_shaders(${LIB} noise)

set(TEST_SRCS
	tests/IslandNoiseTest.cpp
	tests/NoiseTest.cpp
	tests/PoissonDiskDistributionTest.cpp
)
gtest_suite_sources(tests ${TEST_SRCS})
gtest_suite_deps(tests ${LIB} image)

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS} ../core/tests/AbstractTest.cpp)
gtest_suite_deps(tests-${LIB} ${LIB} image)
gtest_suite_end(tests-${LIB})
