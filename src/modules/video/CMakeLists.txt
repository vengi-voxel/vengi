set(VK_SRCS
	vk/flextVk.c vk/flextVk.h
	vk/VkRenderer.cpp vk/VkRenderer.h
	vk/VkShader.cpp
	vk/VkMapping.h
	vk/VkState.h
	vk/VKTypes.h
)
set(GL_SRCS
	gl/flextGL.c gl/flextGL.h
	gl/GLRenderer.cpp gl/GLRenderer.h
	gl/GLMapping.h
	gl/GLShader.cpp
	gl/GLHelper.cpp gl/GLHelper.h
)
set(SRCS
	RendererState.h
	Buffer.cpp Buffer.h
	Camera.cpp Camera.h
	DriverHints.cpp
	FileDialogOptions.h
	FrameBuffer.cpp FrameBuffer.h
	FrameBufferConfig.cpp FrameBufferConfig.h
	OpenFileMode.h
	PenAxis.h
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

set(LIB video)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES image)
if (RENDERER STREQUAL "GL")
	set(OpenGL_GL_PREFERENCE GLVND)
	find_package(OpenGL)
	if (APPLE)
		target_link_libraries(${LIB} PRIVATE "-framework OpenGL -framework CoreFoundation")
	endif()
	set(USE_GL_RENDERER ON CACHE BOOL "Use opengl renderer" FORCE)
elseif (RENDERER STREQUAL "VK")
	find_package(Vulkan REQUIRED)
	if (APPLE)
		target_link_libraries(${LIB} PRIVATE "-framework CoreFoundation")
	endif()
	set(USE_VK_RENDERER ON CACHE BOOL "Use vulkan renderer" FORCE)
endif()
set_target_properties(${LIB} PROPERTIES UNITY_BUILD OFF)

find_program(PYTHON3_EXECUTABLE NAMES python3)
add_custom_target(update-videobindings)
if (PYTHON3_EXECUTABLE)
	add_custom_command(TARGET update-videobindings
		COMMAND ${PYTHON3_EXECUTABLE} flextGLgen.py -T sdl -D ${CMAKE_CURRENT_SOURCE_DIR}/gl profiles/gl33.txt
		WORKING_DIRECTORY ${ROOT_DIR}/tools/flextGL
		POST_BUILD
	)
	add_custom_command(TARGET update-videobindings
		COMMAND ${PYTHON3_EXECUTABLE} flextGLgen.py -T vulkan-dynamic -D ${CMAKE_CURRENT_SOURCE_DIR}/vk profiles/vulkan.txt
		WORKING_DIRECTORY ${ROOT_DIR}/tools/flextGL
		POST_BUILD
	)
endif()

set(TEST_SRCS
	tests/ShaderTest.cpp
	tests/ShapeBuilderTest.cpp
	tests/CameraTest.cpp
	tests/RendererTest.cpp
)

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_deps(tests-${LIB} ${LIB} test-app)
gtest_suite_end(tests-${LIB})

set(BENCHMARK_SRCS
	benchmarks/ShapeBuilderBenchmark.cpp
)
engine_add_executable(TARGET benchmarks-${LIB} SRCS ${BENCHMARK_SRCS} NOINSTALL)
engine_target_link_libraries(TARGET benchmarks-${LIB} DEPENDENCIES benchmark-app ${LIB})
