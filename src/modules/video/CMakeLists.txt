set(SRCS
	Camera.cpp Camera.h
	WindowedApp.cpp WindowedApp.h
	gl/flextGL.c gl/flextGL.h
	gl/GLRenderer.cpp
	gl/GLProfiler.cpp
	gl/GLState.h
	gl/GLMapping.h
	gl/GLShader.cpp
	gl/GLHelper.cpp gl/GLHelper.h
	UniformBuffer.cpp UniformBuffer.h
	VertexBuffer.cpp VertexBuffer.h
	FrameBuffer.cpp FrameBuffer.h
	GBuffer.cpp GBuffer.h
	Shader.cpp Shader.h
	Camera.cpp Camera.h
	DepthBuffer.cpp DepthBuffer.h
	Cubemap.cpp Cubemap.h
	Texture.cpp Texture.h
	Types.h
	Version.h
	Renderer.h
	Mesh.cpp Mesh.h
	MeshPool.cpp MeshPool.h
	ShapeBuilder.cpp ShapeBuilder.h
	ScopedLineWidth.h ScopedLineWidth.cpp
	ScopedViewPort.h
	ScopedFrameBuffer.h
	ScopedScissor.h
	ScopedPolygonMode.h
)
set(LIB video)
find_package(OpenGL REQUIRED)
add_library(${LIB} ${SRCS})
target_link_libraries(${LIB} core image util assimp zlib nativefiledialog ${OPENGL_LIBRARIES})
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})
target_include_directories(${LIB} PUBLIC ${OPENGL_INCLUDE_DIRS})

gtest_suite_files(tests
	tests/ShaderTest.cpp
	tests/CameraTest.cpp
)
gtest_suite_deps(tests ${LIB})
