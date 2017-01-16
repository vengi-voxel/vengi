/**
 * @file
 */

#pragma once

#include "Renderer.h"

namespace video {

class GBuffer {
public:
	enum GBufferTextureType {
		GBUFFER_TEXTURE_TYPE_POSITION, GBUFFER_TEXTURE_TYPE_DIFFUSE, GBUFFER_TEXTURE_TYPE_NORMAL, GBUFFER_NUM_TEXTURES
	};

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
	Id _oldDrawFramebuffer = InvalidId;
	Id _oldReadFramebuffer = InvalidId;
	Id _fbo;
	Id _textures[GBUFFER_NUM_TEXTURES];
	Id _depthTexture;
};

}
