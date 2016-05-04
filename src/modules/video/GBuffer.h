#pragma once

#include "GLFunc.h"

namespace video {

class GBuffer {
public:
	enum GBufferTextureType {
		GBUFFER_TEXTURE_TYPE_POSITION, GBUFFER_TEXTURE_TYPE_DIFFUSE, GBUFFER_TEXTURE_TYPE_NORMAL, GBUFFER_TEXTURE_TYPE_TEXCOORD, GBUFFER_NUM_TEXTURES
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
	GLuint _textures[GBUFFER_NUM_TEXTURES];
	GLuint _depthTexture;
};

}
