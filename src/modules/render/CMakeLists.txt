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
set(SHADERS
	skybox
	color
	color_instanced
	constants
	depthbuffer_render
	edge
	shadowmap
	shadowmap_render
	texture
)
set(SRCS_SHADERS)
foreach (SHADER ${SHADERS})
	list(APPEND SRCS_SHADERS "shaders/${SHADER}.vert")
	list(APPEND SRCS_SHADERS "shaders/${SHADER}.frag")
endforeach()
set(LIB render)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} ${SRCS_SHADERS} DEPENDENCIES video noise)
generate_shaders(${LIB} ${SHADERS})

gtest_suite_sources(tests
	tests/RenderShaderTest.cpp
)
gtest_suite_deps(tests ${LIB} render)
