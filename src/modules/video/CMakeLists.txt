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
	FrameBufferConfig.cpp FrameBufferConfig.h
	GBuffer.cpp GBuffer.h
	Shader.cpp Shader.h
	Camera.cpp Camera.h
	Cubemap.cpp Cubemap.h
	RenderBuffer.cpp RenderBuffer.h
	Texture.cpp Texture.h
	TextureConfig.cpp TextureConfig.h
	TexturePool.cpp TexturePool.h
	Types.h
	Version.h
	Renderer.cpp Renderer.h
	Mesh.cpp Mesh.h
	MeshPool.cpp MeshPool.h
	ShapeBuilder.cpp ShapeBuilder.h
	ShaderManager.cpp ShaderManager.h
	ScopedLineWidth.h ScopedLineWidth.cpp
	ScopedViewPort.h
	ScopedFrameBuffer.h
	ScopedScissor.h
	ScopedPolygonMode.h
	StencilConfig.h StencilConfig.cpp
)
set(LIB video)
find_package(OpenGL)
add_library(${LIB} ${SRCS})
engine_target_link_libraries(TARGET ${LIB} DEPENDENCIES core math image util assimp zlib)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})
target_include_directories(${LIB} PUBLIC ${OPENGL_INCLUDE_DIRS})

gtest_suite_files(tests
	tests/AbstractGLTest.h
	tests/ShaderTest.cpp
	tests/CameraTest.cpp
)
gtest_suite_deps(tests ${LIB})
