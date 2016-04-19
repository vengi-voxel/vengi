fips_begin_module(video)
	fips_files(
		WindowedApp.cpp WindowedApp.h
		GLFunc.cpp GLFunc.h
		VertexBuffer.cpp VertexBuffer.h
		FrameBuffer.cpp FrameBuffer.h
		GBuffer.cpp GBuffer.h
		Shader.cpp Shader.h
		Camera.cpp Camera.h
		GLMeshData.h
		GLDebug.h
		Cubemap.cpp Cubemap.h
		Texture.cpp Texture.h
		Color.cpp Color.h
		Mesh.cpp Mesh.h
		MeshPool.cpp MeshPool.h
	)
	fips_deps(core image io)
fips_end_module()

find_package(assimp REQUIRED)
target_link_libraries(video ${ASSIMP_LIBRARIES})
target_include_directories(video PRIVATE ${ASSIMP_INCLUDE_DIRS})

find_package(OpenGL REQUIRED)
target_link_libraries(video ${OPENGL_LIBRARIES})
target_include_directories(video PUBLIC ${OPENGL_INCLUDE_DIRS})
