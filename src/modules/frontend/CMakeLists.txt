set(SRCS
	Axis.h
	CameraFrustum.cpp CameraFrustum.h
	ClientEntity.h ClientEntity.cpp
	ClientEntityId.h
	GridRenderer.cpp GridRenderer.h
	Movement.h
	Plane.h Plane.cpp
	RandomColorTexture.h RandomColorTexture.cpp
	Shadow.h Shadow.cpp
	ShapeRenderer.cpp ShapeRenderer.h
)

set(LIB frontend)
add_library(${LIB} ${SRCS})
generate_shaders(${LIB} mesh shadowmap shadowmap_instanced shadowmap_render color color_instanced)
engine_target_link_libraries(TARGET ${LIB} DEPENDENCIES network noise cooldown attrib)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})

gtest_suite_files(tests
	tests/FrontendShaderTest.cpp
)
gtest_suite_deps(tests ${LIB} frontend)
