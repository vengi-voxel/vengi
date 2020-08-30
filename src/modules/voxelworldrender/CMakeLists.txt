set(LIB voxelworldrender)
set(SRCS
	WorldRenderer.h WorldRenderer.cpp
	PlayerCamera.cpp PlayerCamera.h

	worldrenderer/WorldChunkMgr.h worldrenderer/WorldChunkMgr.cpp
	worldrenderer/WorldMeshExtractor.h worldrenderer/WorldMeshExtractor.cpp
)
set(SRCS_SHADERS
	shaders/_checker.frag
	shaders/water.vert shaders/water.frag
	shaders/world.vert shaders/world.frag
	shaders/postprocess.vert shaders/postprocess.frag
)
set(FILES
	sky/sky_bk.png
	sky/sky_dn.png
	sky/sky_ft.png
	sky/sky_lf.png
	sky/sky_rt.png
	sky/sky_up.png
	shared/water-distortion.png
	shared/water-normal.png
)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} ${SRCS_SHADERS} FILES ${FILES} DEPENDENCIES frontend voxelrender)
generate_shaders(${LIB} world water postprocess)

set(TEST_SRCS
	tests/VoxelFrontendShaderTest.cpp
)

gtest_suite_sources(tests ${TEST_SRCS})
gtest_suite_deps(tests ${LIB} test-app image)

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_deps(tests-${LIB} ${LIB} test-app image)
gtest_suite_end(tests-${LIB})
