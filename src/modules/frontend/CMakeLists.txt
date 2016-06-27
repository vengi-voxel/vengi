set(SRCS
	ClientEntity.h ClientEntity.cpp
	ClientEntityId.h
	Movement.h
	WorldRenderer.h WorldRenderer.cpp
)

set(LIB frontend)
add_library(${LIB} ${SRCS})
generate_shaders(${LIB} world mesh water world_instanced deferred_light_dir shadowmap shadowmap_render)
target_link_libraries(${LIB} ui network voxel cooldown)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})

gtest_suite_files(tests
	tests/MaterialTest.cpp
	tests/WorldRendererTest.cpp
)
gtest_suite_deps(tests ${LIB} frontend image)
