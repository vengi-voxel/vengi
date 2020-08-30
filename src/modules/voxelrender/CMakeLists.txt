set(LIB voxelrender)
set(SRCS
	CachedMeshRenderer.cpp CachedMeshRenderer.h
	MeshRenderer.cpp MeshRenderer.h
	RawVolumeRenderer.cpp RawVolumeRenderer.h
	ShaderAttribute.h
	VoxelFontRenderer.h VoxelFontRenderer.cpp
)
set(SRCS_SHADERS
	shaders/_material.vert
	shaders/voxel.vert shaders/voxel.frag
	shaders/voxel_indirect.vert shaders/voxel_indirect.frag
)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} ${SRCS_SHADERS} DEPENDENCIES render voxelfont voxelformat)
generate_shaders(${LIB} voxel voxel_indirect)

set(TEST_SRCS
	tests/MaterialTest.cpp
)
gtest_suite_sources(tests ${TEST_SRCS})
gtest_suite_deps(tests ${LIB} test-app image)

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_deps(tests-${LIB} ${LIB} test-app image)
gtest_suite_end(tests-${LIB})
