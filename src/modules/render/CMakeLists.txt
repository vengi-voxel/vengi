set(SRCS
	Axis.cpp Axis.h
	CameraFrustum.cpp CameraFrustum.h
	Gizmo.cpp Gizmo.h
	GridRenderer.cpp GridRenderer.h
	Plane.h Plane.cpp
	RandomColorTexture.h RandomColorTexture.cpp
	Shadow.h Shadow.cpp
	ShapeRenderer.cpp ShapeRenderer.h
	Skybox.h Skybox.cpp
	TextureRenderer.cpp TextureRenderer.h
)
set(SRCS_SHADERS
	shaders/color_instanced.vert shaders/color_instanced.frag
	shaders/color.vert shaders/color.frag
	shaders/shadowmap_instanced.vert shaders/shadowmap_instanced.frag
	shaders/shadowmap_render.vert shaders/shadowmap_render.frag
	shaders/shadowmap.vert shaders/shadowmap.frag
	shaders/skybox.vert shaders/skybox.frag
	shaders/texture.vert shaders/texture.frag
	shaders/constants.vert shaders/constants.frag
)
set(LIB render)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} ${SRCS_SHADERS} DEPENDENCIES video noise)
generate_shaders(${LIB} shadowmap shadowmap_instanced shadowmap_render color color_instanced texture edge skybox constants)

gtest_suite_sources(tests
	tests/RenderShaderTest.cpp
)
gtest_suite_deps(tests ${LIB} render)
