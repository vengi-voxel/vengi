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
)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} ${SRCS_SHADERS} DEPENDENCIES render voxelfont voxelformat)
generate_shaders(${LIB} voxel)

gtest_suite_sources(tests
	tests/MaterialTest.cpp
)
gtest_suite_files(tests shared/worldparams.lua shared/biomes.lua)
gtest_suite_deps(tests ${LIB} image)
