set(SRCS
	Axis.cpp Axis.h
	BloomRenderer.cpp BloomRenderer.h
	CameraFrustum.cpp CameraFrustum.h
	GridRenderer.cpp GridRenderer.h
	Plane.h Plane.cpp
	Shadow.h Shadow.cpp
	ShapeRenderer.cpp ShapeRenderer.h
	TextureRenderer.cpp TextureRenderer.h
)
set(SHADERS
	color
	combine2
	constants
	convolution
	default
	shadowmap
	texture
)
set(SRCS_SHADERS)
foreach (SHADER ${SHADERS})
	list(APPEND SRCS_SHADERS "shaders/${SHADER}.vert")
	list(APPEND SRCS_SHADERS "shaders/${SHADER}.frag")
endforeach()
set(LIB render)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} ${SRCS_SHADERS} DEPENDENCIES video)
generate_shaders(${LIB} ${SHADERS})

set(TEST_SRCS
	tests/RenderShaderTest.cpp
)

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_deps(tests-${LIB} ${LIB} test-app render)
gtest_suite_end(tests-${LIB})
