set(SRCS
	Simplex.h
	Noise.h Noise.cpp
	SphereNoise.h SphereNoise.cpp
	PoissonDiskDistribution.h PoissonDiskDistribution.cpp
)
# TODO: maybe provide two noise modules, one noisefast (for e.g. client only stuff) and one noise-slow for stuff that must be cross plattform

set(LIB noise)
add_library(${LIB} ${SRCS})
engine_target_link_libraries(TARGET ${LIB} DEPENDENCIES core math compute)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})
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
else()
	target_compile_options(${LIB} PRIVATE /Ox)
endif()
generate_compute_shaders(noise noise)

gtest_suite_files(tests
	tests/IslandNoiseTest.cpp
	tests/NoiseTest.cpp
	tests/NoiseShaderTest.cpp
	tests/PoissonDiskDistributionTest.cpp
)
gtest_suite_deps(tests ${LIB} image)

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_files(tests-${LIB} ${TEST_SRCS} ../core/tests/AbstractTest.cpp)
gtest_suite_deps(tests-${LIB} ${LIB} image)
gtest_suite_end(tests-${LIB})
