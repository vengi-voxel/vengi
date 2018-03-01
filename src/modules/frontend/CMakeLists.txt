set(SRCS
	Axis.h
	CameraFrustum.cpp CameraFrustum.h
	ClientEntity.h ClientEntity.cpp
	ClientEntityId.h
	GridRenderer.cpp GridRenderer.h
	MapRenderer.cpp MapRenderer.h
	Movement.h
	OctreeRenderer.cpp OctreeRenderer.h
	Plane.h
	PlantDistributor.cpp PlantDistributor.h
	RandomColorTexture.h RandomColorTexture.cpp
	RawVolumeRenderer.cpp RawVolumeRenderer.h
	ShaderAttribute.h
	Shadow.h Shadow.cpp
	ShapeRenderer.cpp ShapeRenderer.h
	WorldRenderer.h WorldRenderer.cpp
)

set(LIB frontend)
add_library(${LIB} ${SRCS})
generate_shaders(${LIB} world mesh water world_instanced shadowmap shadowmap_instanced shadowmap_render color color_instanced)
engine_target_link_libraries(TARGET ${LIB} DEPENDENCIES network voxel cooldown attrib)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})

gtest_suite_files(tests
	tests/FrontendShaderTest.cpp
	tests/MaterialTest.cpp
	tests/WorldRendererTest.cpp
)
gtest_suite_deps(tests ${LIB} frontend image)
