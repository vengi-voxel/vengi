/**
 * @file
 */

#pragma once

#include "RenderShaders.h"
#include "video/Buffer.h"
#include "video/FrameBuffer.h"
#include <glm/fwd.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>

namespace render {

/**
 * @brief Renders a textures with the shader::BlurShader
 */
class BlurRenderer {
private:
	shader::BlurShader _shader;
	video::Buffer _vbo;
	video::FrameBuffer _frameBuffers[2];

public:
	/**
	 * @sa shutdown()
	 */
	bool init(bool yFlipped, int width = 512, int height = 512);
	/**
	 * @sa init()
	 */
	void shutdown();

	/**
	 * @param srcTextureId The video::Id of the original texture that should get blurred
	 * @note The result is put into a texture that can get queried to continue to use it
	 * @see texture()
	 */
	void render(video::Id srcTextureId, int amount = 10);
	/**
	 * @return video::TexturePtr of the render() pass
	 */
	video::TexturePtr texture() const;
};

} // namespace render
