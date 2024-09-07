set(LIB voxelgenerator)

set(LUA_SRCS
	scripts/modules/perlin.lua
	scripts/modules/volume.lua
	scripts/modules/JSON.lua
	scripts/align.lua
	scripts/animate.lua
	scripts/cover.lua
	scripts/delete-rgb.lua
	scripts/erode.lua
	scripts/fillhollow.lua
	scripts/gradient.lua
	scripts/grass.lua
	scripts/grid.lua
	scripts/imageasvolume.lua
	scripts/maze.lua
	scripts/move.lua
	scripts/noise.lua
	scripts/noise-builtin.lua
	scripts/planet.lua
	scripts/pyramid.lua
	scripts/replacecolor.lua
	scripts/replacepalette.lua
	scripts/resize.lua
	scripts/similarcolor.lua
	scripts/slice.lua
	scripts/splitcolor.lua
	scripts/splitobjects.lua
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
	LUAApi.h LUAApi.cpp
)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} LUA_SRCS ${LUA_SRCS} DEPENDENCIES voxelutil voxelfont noise commonlua scenegraph)

set(TEST_SRCS
	tests/LSystemTest.cpp
	tests/LUAApiTest.cpp
	tests/ShapeGeneratorTest.cpp
)

set(TEST_FILES
	testvoxelgenerator/cone.qb
	testvoxelgenerator/cube.qb
	testvoxelgenerator/cylinder.qb
	testvoxelgenerator/dome.qb
	testvoxelgenerator/ellipse.qb
	tests/test-heightmap-dm.png
	tests/test-heightmap.png
)

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_files(tests-${LIB} ${TEST_FILES})
gtest_suite_lua_sources(tests-${LIB} ${LUA_SRCS})
gtest_suite_deps(tests-${LIB} ${LIB} voxelformat test-app)
gtest_suite_end(tests-${LIB})
