* Fill VkRenderer.cpp functions accordingly to GLRenderer.cpp
* Replace all Shader::setUniform stuff with UBOs
* implement VkShader.cpp according to GLShader.cpp and implement the functions
* support both sdl2 and sdl3 in the vulkan renderer
* replace the GL calls in WindowedApp and guard with USE_GL_RENDERER and USE_VK_RENDERER
