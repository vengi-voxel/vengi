/**
 * @file
 */

#pragma once

#include "GLFunc.h"

namespace video {

class GBuffer {
public:
	enum GBufferTextureType {
		GBUFFER_TEXTURE_TYPE_POSITION, GBUFFER_TEXTURE_TYPE_DIFFUSE, GBUFFER_TEXTURE_TYPE_NORMAL, GBUFFER_NUM_TEXTURES
	};

	GBuffer();
	~GBuffer();

	bool init(int width, int height);
	void shutdown();

	void setReadBuffer(GBufferTextureType textureType);
	void bindForWriting();
	void bindForReading();

private:
	GLuint _fbo;
	// don't change the order - textures are created with one call
	GLuint _textures[GBUFFER_NUM_TEXTURES];
	GLuint _depthTexture;
};

}
