set(LIB voxelgenerator)

set(LUA_SRCS
	scripts/modules/perlin.lua
	scripts/modules/volume.lua
	scripts/modules/JSON.lua
	scripts/modules/stb_aos.lua
	scripts/modules/url.lua
	scripts/modules/util.lua
	scripts/align.lua
	scripts/animate.lua
	scripts/aos_caves.lua
	scripts/aos_forest_river.lua
	scripts/aos_incomplete_fortress.lua
	scripts/aos_rainbow_towers.lua
	scripts/aos_sinc.lua
	scripts/aos_slope.lua
	scripts/compaction.lua
	scripts/cover.lua
	scripts/delete-rgb.lua
	scripts/erode.lua
	scripts/fillhollow.lua
	scripts/fillz.lua
	scripts/flatten.lua
	scripts/gameoflife.lua
	scripts/genland.lua
	scripts/gradient.lua
	scripts/grass.lua
	scripts/grid.lua
	scripts/imageasvolume.lua
	scripts/mandelbulb.lua
	scripts/maze.lua
	scripts/meshy.lua
	scripts/move.lua
	scripts/newelstair.lua
	scripts/noise.lua
	scripts/noise-builtin.lua
	scripts/planet.lua
	scripts/pyramid.lua
	scripts/remapcolors.lua
	scripts/replacecolor.lua
	scripts/replacepalette.lua
	scripts/resize.lua
	scripts/shadow.lua
	scripts/similarcolor.lua
	scripts/slice.lua
	scripts/smooth.lua
	scripts/splitcolor.lua
	scripts/splitobjects.lua
	scripts/thicken.lua
)
engine_add_module(TARGET ${LIB}-lua LUA_SRCS ${LUA_SRCS})

set(SRCS
	Genland.h Genland.cpp
	Spiral.h
	ShapeGenerator.h
	SpaceColonization.h SpaceColonization.cpp
	TreeType.h
	TreeGenerator.h TreeGenerator.cpp
	TreeContext.h
	LSystem.h LSystem.cpp
	LUAApi.h LUAApi.cpp
)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES voxelutil voxelfont noise commonlua scenegraph)

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
gtest_suite_deps(tests-${LIB} ${LIB} voxelformat ${LIB}-lua test-app)
gtest_suite_end(tests-${LIB})

set(BENCHMARK_SRCS
	benchmarks/ShapeGeneratorBenchmark.cpp
)
engine_add_executable(TARGET benchmarks-${LIB} SRCS ${BENCHMARK_SRCS} NOINSTALL)
engine_target_link_libraries(TARGET benchmarks-${LIB} DEPENDENCIES benchmark-app ${LIB})
