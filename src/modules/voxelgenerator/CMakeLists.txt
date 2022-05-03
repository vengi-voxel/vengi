set(LIB voxelgenerator)

set(LUA_SRCS
	scripts/modules/perlin.lua
	scripts/modules/volume.lua
	scripts/cover.lua
	scripts/delete-rgb.lua
	scripts/grass.lua
	scripts/grid.lua
	scripts/noise.lua
	scripts/noise-builtin.lua
	scripts/planet.lua
	scripts/pyramid.lua
	scripts/replacecolor.lua
	scripts/similarcolor.lua
	scripts/thicken.lua
)

set(SRCS
	Spiral.h
	ShapeGenerator.h
	SpaceColonization.h SpaceColonization.cpp
	TreeType.h
	TreeGenerator.h TreeGenerator.cpp
	TreeContext.h
	LSystem.h LSystem.cpp
	LUAGenerator.h LUAGenerator.cpp
)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} LUA_SRCS ${LUA_SRCS} DEPENDENCIES voxelutil noise commonlua)

set(TEST_SRCS
	tests/LSystemTest.cpp
	tests/LUAGeneratorTest.cpp
	tests/ShapeGeneratorTest.cpp
)

set(TEST_FILES
	testvoxelgenerator/cone.qb
	testvoxelgenerator/cube.qb
	testvoxelgenerator/cylinder.qb
	testvoxelgenerator/dome.qb
	testvoxelgenerator/ellipse.qb
)

set(TEST_DEPENDENCIES voxelformat test-app)

gtest_suite_sources(tests ${TEST_SRCS})
gtest_suite_files(tests ${TEST_FILES})
gtest_suite_deps(tests ${LIB} ${TEST_DEPENDENCIES})

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_files(tests-${LIB} ${TEST_FILES})
gtest_suite_lua_sources(tests-${LIB} ${LUA_SRCS})
gtest_suite_deps(tests-${LIB} ${LIB} ${TEST_DEPENDENCIES})
gtest_suite_end(tests-${LIB})
