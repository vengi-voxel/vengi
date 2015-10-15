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
		Image.cpp Image.h
		Cubemap.cpp Cubemap.h
		Texture.cpp Texture.h
		Mesh.cpp Mesh.h
		MeshPool.cpp MeshPool.h
	)
	find_package(OpenGL)
	find_package(SDL2 REQUIRED)
	include_directories(${SDL2_INCLUDE_DIRS} ${OPENGL_INCLUDE_DIR})
	fips_deps(core io assimp ${SDL2_LIBRARIES} ${OPENGL_LIBRARIES})
fips_end_module()
