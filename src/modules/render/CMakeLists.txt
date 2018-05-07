set(SRCS
	Axis.h
	CameraFrustum.cpp CameraFrustum.h
	GridRenderer.cpp GridRenderer.h
	Plane.h Plane.cpp
	RandomColorTexture.h RandomColorTexture.cpp
	Shadow.h Shadow.cpp
	ShapeRenderer.cpp ShapeRenderer.h
	TextureRenderer.cpp TextureRenderer.h
)

set(LIB render)
add_library(${LIB} ${SRCS})
generate_shaders(${LIB} mesh shadowmap shadowmap_instanced shadowmap_render color color_instanced texture)
engine_target_link_libraries(TARGET ${LIB} DEPENDENCIES video noise)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})

gtest_suite_files(tests
	tests/RenderShaderTest.cpp
)
gtest_suite_deps(tests ${LIB} render)
