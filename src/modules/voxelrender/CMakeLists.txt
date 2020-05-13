set(LIB voxelrender)
set(SRCS
	CachedMeshRenderer.cpp CachedMeshRenderer.h
	MeshRenderer.cpp MeshRenderer.h
	RawVolumeRenderer.cpp RawVolumeRenderer.h
	PlayerCamera.cpp PlayerCamera.h
	ShaderAttribute.h
	VoxelFontRenderer.h VoxelFontRenderer.cpp
	WorldRenderer.h WorldRenderer.cpp

	worldrenderer/WorldBuffers.h worldrenderer/WorldBuffers.cpp
	worldrenderer/WorldChunkMgr.h worldrenderer/WorldChunkMgr.cpp
	worldrenderer/WorldMeshExtractor.h worldrenderer/WorldMeshExtractor.cpp
)
set(SRCS_SHADERS
	shaders/_checker.frag
	shaders/_material.vert
	shaders/water.vert shaders/water.frag
	shaders/world.vert shaders/world.frag
	shaders/voxel.vert shaders/voxel.frag
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
engine_add_module(TARGET ${LIB} SRCS ${SRCS} ${SRCS_SHADERS} FILES ${FILES} DEPENDENCIES frontend render voxelfont)
generate_shaders(${LIB} world water voxel postprocess)

gtest_suite_sources(tests
	tests/VoxelFrontendShaderTest.cpp
	tests/MaterialTest.cpp
)
gtest_suite_files(tests shared/worldparams.lua shared/biomes.lua)
gtest_suite_deps(tests ${LIB} voxelrender image)
