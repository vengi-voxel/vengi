set(SRCS
	Camera.cpp Camera.h
	WindowedApp.cpp WindowedApp.h
	GLFunc.cpp GLFunc.h
	UniformBuffer.cpp UniformBuffer.h
	VertexBuffer.cpp VertexBuffer.h
	FrameBuffer.cpp FrameBuffer.h
	GBuffer.cpp GBuffer.h
	Shader.cpp Shader.h
	Camera.cpp Camera.h
	GLMeshData.h
	GLDebug.h
	DepthBuffer.cpp DepthBuffer.h
	Cubemap.cpp Cubemap.h
	Texture.cpp Texture.h
	Mesh.cpp Mesh.h
	MeshPool.cpp MeshPool.h
	RectTesselator.cpp RectTesselator.h
	ScopedViewPort.h
	ScopedFrameBuffer.h
	SunLight.h SunLight.cpp
)
set(LIB video)
find_package(OpenGL REQUIRED)
add_library(${LIB} ${SRCS})
target_link_libraries(${LIB} core image assimp zlib ${OPENGL_LIBRARIES})
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})
target_include_directories(${LIB} PUBLIC ${OPENGL_INCLUDE_DIRS})

gtest_suite_files(tests
	tests/ShaderTest.cpp
	tests/CameraTest.cpp
)
gtest_suite_deps(tests ${LIB})
