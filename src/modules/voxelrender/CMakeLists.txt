set(LIB voxelrender)
set(SRCS
	MapRenderer.cpp MapRenderer.h
	PlantDistributor.cpp PlantDistributor.h
	RawVolumeRenderer.cpp RawVolumeRenderer.h
	PagedVolumeRenderer.cpp PagedVolumeRenderer.h
	ShaderAttribute.h
	VoxelFontRenderer.h VoxelFontRenderer.cpp
	WorldRenderer.h WorldRenderer.cpp
)
set(SRCS_SHADERS
	shaders/water.vert shaders/water.frag
	shaders/world_instanced.vert shaders/world_instanced.frag
	shaders/world.vert shaders/world.frag
)
set(FILES
)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} ${SRCS_SHADERS} FILES ${FILES} DEPENDENCIES frontend render voxelfont)
generate_shaders(${LIB} world water world_instanced)

gtest_suite_sources(tests
	tests/VoxelFrontendShaderTest.cpp
	tests/MaterialTest.cpp
	tests/WorldRendererTest.cpp
)
gtest_suite_files(tests shared/worldparams.lua)
gtest_suite_deps(tests ${LIB} voxelrender image)
