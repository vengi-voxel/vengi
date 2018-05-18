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
set(SRCS_SHADERS
	shaders/color_instanced.vert shaders/color_instanced.frag
	shaders/color.vert shaders/color.frag
	shaders/mesh.vert shaders/mesh.frag
	shaders/shadowmap_instanced.vert shaders/shadowmap_instanced.frag
	shaders/shadowmap_render.vert shaders/shadowmap_render.frag
	shaders/shadowmap.vert shaders/shadowmap.frag
	shaders/texture.vert shaders/texture.frag
)
#set_source_files_properties(${SRCS_SHADERS} PROPERTIES LANGUAGE C)
set(LIB render)
add_library(${LIB} ${SRCS} ${SRCS_SHADERS})
generate_shaders(${LIB} mesh shadowmap shadowmap_instanced shadowmap_render color color_instanced texture)
engine_target_link_libraries(TARGET ${LIB} DEPENDENCIES video noise)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})

gtest_suite_files(tests
	tests/RenderShaderTest.cpp
)
gtest_suite_deps(tests ${LIB} render)
