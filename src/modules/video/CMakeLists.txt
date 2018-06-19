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
	Buffer.cpp Buffer.h
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
	ScopedState.h
	ScopedBlendMode.h ScopedBlendMode.cpp
	ScopedScissor.h
	ScopedPolygonMode.h
	StencilConfig.h StencilConfig.cpp
)
set(SRCS_SHADERS
	shaders/_fog.vert shaders/_fog.frag
	shaders/_fullscreen.vert
	shaders/_shadowmap.vert shaders/_shadowmap.frag
)
#set_source_files_properties(${SRCS_SHADERS} PROPERTIES LANGUAGE C)
set(LIB video)
set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL)
add_library(${LIB} ${SRCS} ${SRCS_SHADERS})
engine_target_link_libraries(TARGET ${LIB} DEPENDENCIES core math image util assimp zlib)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})

find_program(PYTHON3_EXECUTABLE NAMES python3)
add_custom_target(update-glbindings)
if (PYTHON3_EXECUTABLE)
	add_custom_command(TARGET update-glbindings
		COMMAND ${PYTHON3_EXECUTABLE} flextGLgen.py -T sdl -D ${CMAKE_CURRENT_SOURCE_DIR}/gl flextgl_profile.txt
		DEPENDS flextgl_profile.txt ${ROOT_DIR}/tools/flextGL/flextGLgen.py
		WORKING_DIRECTORY ${ROOT_DIR}/tools/flextGL
	)
endif()

gtest_suite_files(tests
	tests/AbstractGLTest.h
	tests/ShaderTest.cpp
	tests/CameraTest.cpp
)
gtest_suite_deps(tests ${LIB})
