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
	shader::TextureShader _textureShader;
	video::Buffer _texturedFullscreenQuad;
public:
	/**
	 * @brief Initializes the shader and the vbo for the texture rendering
	 * @param[in] size The size for the texture quad
	 * @sa shutdown()
	 */
	bool init(const glm::vec2& size);
	/**
	 * @sa init()
	 */
	void shutdown();

	/**
	 * @brief Perform the rendering with the shader::TextureShader
	 * @note The given texture unit must have the texture bound already
	 * @param[in] projection The projection matrix to hand over to the texture shader
	 * @param[in] model The model matrix to hand over to the texture shader
	 */
	void render(const glm::mat4& projection, const glm::mat4& model = glm::mat4(1.0f), video::TextureUnit texUnit = video::TextureUnit::Zero);
};

}
