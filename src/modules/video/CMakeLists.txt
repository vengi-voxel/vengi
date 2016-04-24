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
		Remotery.c Remotery.h
	)
	fips_deps(core image io assimp sdl2 ${CMAKE_DL_LIBS})
fips_end_module()
set_target_properties(video PROPERTIES COMPILE_FLAGS "-DUSE_REMOTERY=1 -DRMT_USE_OPENGL")

find_package(OpenGL REQUIRED)
target_link_libraries(video ${OPENGL_LIBRARIES})
target_include_directories(video PUBLIC ${OPENGL_INCLUDE_DIRS})
