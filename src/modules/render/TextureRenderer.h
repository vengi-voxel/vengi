/**
 * @file
 */

#pragma once

#include "video/VertexBuffer.h"
#include "RenderShaders.h"
#include <glm/fwd.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>

namespace render {

class TextureRenderer {
private:
	shader::TextureShader _textureShader;
	video::VertexBuffer _texturedFullscreenQuad;
public:
	bool init(const glm::vec2& size);
	void shutdown();

	void render(const glm::mat4& projection);
};

}
