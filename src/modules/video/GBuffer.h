/**
 * @file
 */

#pragma once

#include "Renderer.h"

namespace video {

/**
 * @ingroup Video
 */
class GBuffer {
public:
	GBuffer();
	~GBuffer();

	bool init(const glm::ivec2& dimension);
	void shutdown();

	void setReadBuffer(GBufferTextureType textureType);
	void bindForWriting();
	/**
	 * @param[in] gbuffer If true, the gbuffer fbo is bound, if this is @c false, the single
	 * textures of the gbuffer are bound to the video::TextureUnit::One onward.
	 */
	void bindForReading(bool gbuffer = false);
	void unbind();

private:
	Id _fbo;
	Id _textures[GBUFFER_NUM_TEXTURES];
	Id _depthTexture;
};

}
