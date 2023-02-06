set(VK_SRCS
	vk/flextVk.c vk/flextVk.h
	vk/VkRenderer.cpp vk/VkRenderer.h
	vk/VkShader.cpp
)
set(GL_SRCS
	gl/flextGL.c gl/flextGL.h
	gl/GLRenderer.cpp gl/GLRenderer.h
	gl/GLState.h
	gl/GLMapping.h
	gl/GLShader.cpp
	gl/GLHelper.cpp gl/GLHelper.h
)
set(SRCS
	Buffer.cpp Buffer.h
	BufferLockMgr.cpp BufferLockMgr.h
	Camera.cpp Camera.h
	Cubemap.cpp Cubemap.h
	DriverHints.cpp
	FileDialogOptions.h
	FrameBuffer.cpp FrameBuffer.h
	FrameBufferConfig.cpp FrameBufferConfig.h
	GBuffer.cpp GBuffer.h
	IndirectDrawBuffer.cpp IndirectDrawBuffer.h
	OpenFileMode.h
	PersistentMappingBuffer.cpp PersistentMappingBuffer.h
	Renderer.cpp Renderer.h
	RendererInterface.h
	RenderBuffer.cpp RenderBuffer.h
	Shader.cpp Shader.h
	ShaderTypes.h
	ShapeBuilder.cpp ShapeBuilder.h
	ShaderManager.cpp ShaderManager.h
	ScopedLineWidth.h ScopedLineWidth.cpp
	ScopedViewPort.h ScopedViewPort.cpp
	ScopedFrameBuffer.h ScopedFrameBuffer.cpp
	ScopedState.h ScopedState.cpp
	ScopedBlendMode.h ScopedBlendMode.cpp
	ScopedRenderState.h ScopedRenderState.cpp
	ScopedScissor.h ScopedScissor.cpp
	ScopedPolygonMode.h ScopedPolygonMode.cpp
	ScopedRenderBuffer.h ScopedRenderBuffer.cpp
	ShaderStorageBuffer.h ShaderStorageBuffer.cpp
	StencilConfig.h StencilConfig.cpp
	Texture.cpp Texture.h
	TextureConfig.cpp TextureConfig.h
	TexturePool.cpp TexturePool.h
	Trace.cpp Trace.h
	Types.h
	UniformBuffer.cpp UniformBuffer.h
	Version.h
	WindowedApp.cpp WindowedApp.h

	EventHandler.cpp EventHandler.h
	IEventObserver.h
)
set(RENDERER GL)
#set(RENDERER VK)
list(APPEND SRCS ${${RENDERER}_SRCS})

if (APPLE)
	list(APPEND SRCS WindowedAppOSX.m)
endif()

set(SRCS_SHADERS
	shaders/_fullscreen.vert
	shaders/_shadowmap.vert shaders/_shadowmap.frag
)
set(FILES
)

#set_source_files_properties(${SRCS_SHADERS} PROPERTIES LANGUAGE C)
set(LIB video)
set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} ${SRCS_SHADERS} FILES ${FILES} DEPENDENCIES image util)
if (APPLE)
	target_link_libraries(${LIB} PRIVATE "-framework OpenGL -framework CoreFoundation")
endif()
set_target_properties(${LIB} PROPERTIES UNITY_BUILD OFF)

find_program(PYTHON3_EXECUTABLE NAMES python3)
add_custom_target(update-videobindings)
if (PYTHON3_EXECUTABLE)
	add_custom_command(TARGET update-videobindings
		COMMAND ${PYTHON3_EXECUTABLE} flextGLgen.py -T sdl -D ${CMAKE_CURRENT_SOURCE_DIR}/gl profiles/gl33.txt
		DEPENDS profiles/gl33.txt ${ROOT_DIR}/tools/flextGL/flextGLgen.py ${ROOT_DIR}/tools/flextGL/templates/sdl/flextGL.c.template ${ROOT_DIR}/tools/flextGL/templates/sdl/flextGL.h.template
		WORKING_DIRECTORY ${ROOT_DIR}/tools/flextGL
	)
	add_custom_command(TARGET update-videobindings
		COMMAND ${PYTHON3_EXECUTABLE} flextGLgen.py -T vulkan-dynamic -D ${CMAKE_CURRENT_SOURCE_DIR}/vk profiles/vulkan.txt
		DEPENDS profiles/vulkan.txt ${ROOT_DIR}/tools/flextGL/flextGLgen.py ${ROOT_DIR}/tools/flextGL/templates/vulkan-dynamic/flextVk.c.template ${ROOT_DIR}/tools/flextGL/templates/vulkan-dynamic/flextVk.h.template
		WORKING_DIRECTORY ${ROOT_DIR}/tools/flextGL
	)
endif()

set(TEST_SRCS
	tests/AbstractGLTest.h
	tests/ShaderTest.cpp
	tests/CameraTest.cpp
	tests/RendererTest.cpp
)

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_deps(tests-${LIB} ${LIB} test-app)
gtest_suite_end(tests-${LIB})
