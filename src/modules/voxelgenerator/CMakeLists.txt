set(LIB voxelgenerator)
set(SRCS
	Spiral.h
	ShapeGenerator.h
	SpaceColonization.h SpaceColonization.cpp
	TreeType.h
	TreeGenerator.h TreeGenerator.cpp
	TreeContext.h
	LSystem.h LSystem.cpp
	LUAGenerator.h LUAGenerator.cpp
	NoiseGenerator.h NoiseGenerator.cpp
)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES voxel voxelutil noise commonlua)

set(TEST_SRCS
	tests/LSystemTest.cpp
	tests/LUAGeneratorTest.cpp
)

gtest_suite_sources(tests ${TEST_SRCS})
gtest_suite_deps(tests ${LIB})

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS} ../core/tests/AbstractTest.cpp)
gtest_suite_deps(tests-${LIB} ${LIB})
gtest_suite_end(tests-${LIB})
