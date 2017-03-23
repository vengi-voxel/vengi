set(SRCS
	Noise.h Noise.cpp
	SphereNoise.h SphereNoise.cpp
	PoissonDiskDistribution.h PoissonDiskDistribution.cpp
)
# TODO: maybe provide two noise modules, one noisefast (for e.g. client only stuff) and one noise-slow for stuff that must be cross plattform

set(LIB noise)
add_library(${LIB} ${SRCS})
target_link_libraries(${LIB} core)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})
#set(MARCH native)
set(MARCH generic)
# http://christian-seiler.de/projekte/fpmath/
if (NOT MSVC)
	target_compile_options(${LIB} PRIVATE -O3 PRIVATE -mtune=${MARCH} PRIVATE -ffloat-store)
else()
	# TODO: MSVC
endif()

gtest_suite_files(tests
	tests/NoiseTest.cpp
	tests/PoissonDiskDistributionTest.cpp
)
gtest_suite_deps(tests ${LIB} image)

gtest_suite_begin(tests-noise TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_files(tests-noise
	../core/tests/AbstractTest.cpp
	tests/NoiseTest.cpp
	tests/PoissonDiskDistributionTest.cpp
)
gtest_suite_deps(tests-noise ${LIB} image)
gtest_suite_end(tests-noise)
