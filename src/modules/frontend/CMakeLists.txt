set(SRCS
	ClientEntity.h ClientEntity.cpp
	ClientEntityId.h
	Movement.h
	CameraFrustum.cpp CameraFrustum.h
	ShapeRenderer.cpp ShapeRenderer.h
	PlantDistributor.cpp PlantDistributor.h
	RawVolumeRenderer.cpp RawVolumeRenderer.h
	Axis.h
	Plane.h
	Shadow.h Shadow.cpp
	WorldRenderer.h WorldRenderer.cpp
)

set(LIB frontend)
add_library(${LIB} ${SRCS})
generate_shaders(${LIB} world mesh water world_instanced deferred_light_dir shadowmap shadowmap_instanced shadowmap_render color)
target_link_libraries(${LIB} ui network voxel cooldown attrib)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})

gtest_suite_files(tests
	tests/MaterialTest.cpp
	tests/WorldRendererTest.cpp
)
gtest_suite_deps(tests ${LIB} frontend image)
