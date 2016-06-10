/**
 * @file
 */

#include "GBuffer.h"

#include <cstddef>
#include "core/Common.h"

namespace video {

GBuffer::GBuffer() :
		_fbo(0), _depthTexture(0) {
	for (std::size_t i = 0; i < SDL_arraysize(_textures); ++i) {
		_textures[i] = 0;
	}
}

GBuffer::~GBuffer() {
	shutdown();
}

void GBuffer::shutdown() {
	if (_fbo != 0) {
		glDeleteFramebuffers(1, &_fbo);
		_fbo = 0;
	}

	if (_textures[0] != 0) {
		glDeleteTextures(SDL_arraysize(_textures), _textures);
		for (int i = 0; i < (int)SDL_arraysize(_textures); ++i) {
			_textures[i] = 0;
		}
	}

	if (_depthTexture != 0) {
		glDeleteTextures(1, &_depthTexture);
		_depthTexture = 0;
	}
}

bool GBuffer::init(int width, int height) {
	glGenFramebuffers(1, &_fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _fbo);

	// +1 for the depth texture
	glGenTextures(SDL_arraysize(_textures) + 1, _textures);

	for (std::size_t i = 0; i < SDL_arraysize(_textures); ++i) {
		glBindTexture(GL_TEXTURE_2D, _textures[i]);
		// we are going to write vec3 into the out vars in the shaders
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, nullptr);
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, _textures[i], 0);
		GL_checkError();
	}

	glBindTexture(GL_TEXTURE_2D, _depthTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _depthTexture, 0);
	GL_checkError();

	const GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	static_assert(SDL_arraysize(drawBuffers) == SDL_arraysize(_textures), "buffers and textures don't match");
	glDrawBuffers(SDL_arraysize(drawBuffers), drawBuffers);
	GL_checkError();

	const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	// restore default FBO
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	GL_checkError();

	if (status != GL_FRAMEBUFFER_COMPLETE) {
		Log::error("FB error, status: %i", (int)status);
		return false;
	}

	return true;
}

void GBuffer::bindForWriting() {
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _fbo);
#ifdef DEBUG
	const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		Log::error("Failed to bind framebuffer for writing");
	}
#endif
	GL_checkError();
}

void GBuffer::bindForReading() {
	glBindFramebuffer(GL_READ_FRAMEBUFFER, _fbo);
#ifdef DEBUG
	const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		Log::error("Failed to bind framebuffer for reading");
	}
	GL_checkError();
#endif
}

void GBuffer::setReadBuffer(GBufferTextureType textureType) {
	glReadBuffer(GL_COLOR_ATTACHMENT0 + textureType);
#ifdef DEBUG
	const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		Log::error("Failed to set the read buffer to %i", (int)textureType);
	}
#endif
	GL_checkError();
}

}
