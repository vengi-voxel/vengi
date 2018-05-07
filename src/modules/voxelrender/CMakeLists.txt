set(SRCS
	MapRenderer.cpp MapRenderer.h
	OctreeRenderer.cpp OctreeRenderer.h
	PlantDistributor.cpp PlantDistributor.h
	RawVolumeRenderer.cpp RawVolumeRenderer.h
	PagedVolumeRenderer.cpp PagedVolumeRenderer.h
	ShaderAttribute.h
	WorldRenderer.h WorldRenderer.cpp
)

set(LIB voxelrender)
add_library(${LIB} ${SRCS})
generate_shaders(${LIB} world water world_instanced)
engine_target_link_libraries(TARGET ${LIB} DEPENDENCIES frontend render voxel)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})

gtest_suite_files(tests
	tests/VoxelFrontendShaderTest.cpp
	tests/MaterialTest.cpp
	tests/WorldRendererTest.cpp
)
gtest_suite_deps(tests ${LIB} voxelrender image)
