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

set(TEST_SRCS
	tests/RenderShaderTest.cpp
)

gtest_suite_sources(tests ${TEST_SRCS})
gtest_suite_deps(tests ${LIB} test-app render)

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_deps(tests-${LIB} ${LIB} test-app render)
gtest_suite_end(tests-${LIB})
