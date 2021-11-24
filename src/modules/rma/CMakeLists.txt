set(SRCS
	MapBuilder.h MapBuilder.cpp
	MetaMap.h MetaMap.cpp
	LUAMetaMap.h LUAMetaMap.cpp
)

set(FILES
	rma/maps/house1_0.qb
	rma/maps/house1_1.qb
	rma/maps/street_h_1_0.qb
	rma/maps/street_v_1_0.qb
	rma/maps/street_cross_1_0.qb
)

set(LUA_SRCS
	tiles.lua
)

set(LIB rma)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} LUA_SRCS ${LUA_SRCS} ${SRCS_SHADERS} FILES ${FILES} DEPENDENCIES math voxelutil commonlua voxelformat)

set(TEST_SRCS
	tests/LUAMetaMapTest.cpp
	tests/MapBuilderTest.cpp
)

gtest_suite_sources(tests ${TEST_SRCS})
gtest_suite_deps(tests ${LIB} test-app rma)

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_deps(tests-${LIB} ${LIB} test-app rma)
gtest_suite_end(tests-${LIB})
