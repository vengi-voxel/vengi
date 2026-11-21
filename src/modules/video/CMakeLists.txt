set(VK_SRCS
	vk/flextVk.c vk/flextVk.h
	vk/VkRenderer.cpp vk/VkRenderer.h
	vk/VkMapping.h vk/VkMemoryRanges.h
	vk/VKTypes.h
)
set(GL_SRCS
	gl/flextGL.c gl/flextGL.h
	gl/GLRenderer.cpp
	gl/GLTypes.h
	gl/GLVersion.h
)
set(SRCS
	RendererState.h
	Buffer.cpp Buffer.h
	Camera.cpp Camera.h
	DriverHints.cpp
	FileDialogOptions.h
	FrameBuffer.cpp FrameBuffer.h
	FrameBufferConfig.cpp FrameBufferConfig.h
	KeyboardLayout.cpp KeyboardLayout.h
	OpenFileMode.h
	PenAxis.h
	Renderer.cpp Renderer.h
	RendererInterface.cpp RendererInterface.h
	RendererBackend.h RendererBackendMethods.inl
	RenderStats.cpp RenderStats.h
	DeferredDestroy.cpp DeferredDestroy.h
	UploadRing.cpp UploadRing.h
	RendererState.cpp RendererState.h
	RenderBuffer.cpp RenderBuffer.h
	SamplerConfig.h
	Shader.cpp Shader.h
	ShaderTypes.h
	ShapeBuilder.cpp ShapeBuilder.h
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
	ShaderStorageBuffer.cpp ShaderStorageBuffer.h
	Version.h
	WindowedApp.cpp WindowedApp.h

	EventHandler.cpp EventHandler.h
	IEventObserver.h
)
# Native builds with glslangValidator can embed Vulkan shaders alongside GLSL.
if (NOT DEFINED CACHE{VIDEO_RENDERER})
	if (GLSLANG_EXECUTABLE AND NOT CMAKE_CROSSCOMPILING AND NOT USE_OPENGLES)
		set(VIDEO_RENDERER "BOTH")
	else()
		set(VIDEO_RENDERER "GL")
	endif()
endif()
set(VIDEO_RENDERER "${VIDEO_RENDERER}" CACHE STRING "Build renderer backends (GL, VK or BOTH)" FORCE)
set_property(CACHE VIDEO_RENDERER PROPERTY STRINGS GL VK BOTH)
if (NOT VIDEO_RENDERER MATCHES "^(GL|VK|BOTH)$")
	message(FATAL_ERROR "VIDEO_RENDERER must be GL, VK or BOTH (got '${VIDEO_RENDERER}')")
endif()

set(USE_GL_RENDERER OFF CACHE BOOL "Build OpenGL renderer" FORCE)
set(USE_VK_RENDERER OFF CACHE BOOL "Build Vulkan renderer" FORCE)
if (VIDEO_RENDERER STREQUAL "GL" OR VIDEO_RENDERER STREQUAL "BOTH")
	list(APPEND SRCS ${GL_SRCS})
	set(USE_GL_RENDERER ON CACHE BOOL "Build OpenGL renderer" FORCE)
endif()
if (VIDEO_RENDERER STREQUAL "VK" OR VIDEO_RENDERER STREQUAL "BOTH")
	if (NOT GLSLANG_EXECUTABLE)
		message(FATAL_ERROR "Vulkan renderer requires glslangValidator for SPIR-V shader compilation")
	endif()
	list(APPEND SRCS ${VK_SRCS})
	set(USE_VK_RENDERER ON CACHE BOOL "Build Vulkan renderer" FORCE)
	set(USE_SPIRV ON CACHE BOOL "Enable SPIR-V shader compilation (required for Vulkan)" FORCE)
endif()

set(LIB video)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES image)
if (USE_GL_RENDERER)
	set(OpenGL_GL_PREFERENCE GLVND)
	find_package(OpenGL)
	if (APPLE)
		target_link_libraries(${LIB} PRIVATE "-framework OpenGL")
	elseif (ANDROID)
		target_link_libraries(${LIB} PUBLIC GLESv3 EGL)
	endif()
endif()
if (APPLE)
	target_link_libraries(${LIB} PRIVATE "-framework CoreFoundation")
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
	tests/FrameBufferTest.cpp
	tests/WindowedAppTest.cpp
)

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_deps(tests-${LIB} ${LIB} test-app)
gtest_suite_end(tests-${LIB})

if (USE_BENCHMARKS)
	set(BENCHMARK_SRCS
		benchmarks/ShapeBuilderBenchmark.cpp
	)
	engine_add_executable(TARGET benchmarks-${LIB} SRCS ${BENCHMARK_SRCS} NOINSTALL)
	engine_target_link_libraries(TARGET benchmarks-${LIB} DEPENDENCIES benchmark-app ${LIB})
endif()
