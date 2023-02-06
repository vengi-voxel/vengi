/**
 * @file
 */

#pragma once

#include "video/Buffer.h"
#include "RenderShaders.h"
#include <glm/fwd.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>

namespace render {

/**
 * @brief Renders a textures with the shader::TextureShader
 */
class TextureRenderer {
private:
	shader::TextureShader _shader;
	video::Buffer _vbo;
public:
	/**
	 * @brief Initializes the shader and the vbo for the texture rendering
	 * @sa shutdown()
	 */
	bool init(bool yFlipped = false);
	/**
	 * @sa init()
	 */
	void shutdown();

	/**
	 * @brief Perform the rendering with the shader::TextureShader (of the whole texture)
	 * @note The given texture unit must have the texture bound already
	 */
	void render(video::TextureUnit texUnit = video::TextureUnit::Zero);
};

}
