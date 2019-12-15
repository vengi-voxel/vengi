set(LIB voxelrender)
set(SRCS
	MapRenderer.cpp MapRenderer.h
	RawVolumeRenderer.cpp RawVolumeRenderer.h
	PagedVolumeRenderer.cpp PagedVolumeRenderer.h
	PlayerCamera.cpp PlayerCamera.h
	ShaderAttribute.h
	VoxelFontRenderer.h VoxelFontRenderer.cpp
	WorldRenderer.h WorldRenderer.cpp
)
set(SRCS_SHADERS
	shaders/water.vert shaders/water.frag
	shaders/world_instanced.vert shaders/world_instanced.frag
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
)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} ${SRCS_SHADERS} FILES ${FILES} DEPENDENCIES frontend render voxelfont voxelworld)
generate_shaders(${LIB} world water world_instanced voxel postprocess)

gtest_suite_sources(tests
	tests/VoxelFrontendShaderTest.cpp
	tests/MaterialTest.cpp
	tests/WorldRendererTest.cpp
)
gtest_suite_files(tests shared/worldparams.lua shared/biomes.lua)
gtest_suite_deps(tests ${LIB} voxelrender image)
