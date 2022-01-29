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
 * @brief Renders two textures with the shader::BloomShader
 * @see BlurRenderer
 */
class BloomRenderer {
private:
	shader::BloomShader _shader;
	video::Buffer _vbo;
public:
	/**
	 * @sa shutdown()
	 */
	bool init(bool yFlipped);
	/**
	 * @sa init()
	 */
	void shutdown();

	/**
	 * @param color0 The color texture
	 * @param color1 The bloom texture to blend
	 */
	void render(video::Id color0, video::Id color1, bool bloom = true);
};

}
