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
	}

	if (_depthTexture != 0) {
		glDeleteTextures(1, &_depthTexture);
	}
}

bool GBuffer::init(int width, int height) {
	glGenFramebuffers(1, &_fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _fbo);

	// +1 for the depth texture
	glGenTextures(SDL_arraysize(_textures) + 1, _textures);

	for (std::size_t i = 0; i < SDL_arraysize(_textures); ++i) {
		glBindTexture(GL_TEXTURE_2D, _textures[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, nullptr);
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, _textures[i], 0);
	}

	glBindTexture(GL_TEXTURE_2D, _depthTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _depthTexture, 0);

	const GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
	glDrawBuffers(SDL_arraysize(drawBuffers), drawBuffers);

	const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	// restore default FBO
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

	if (status != GL_FRAMEBUFFER_COMPLETE) {
		Log::error("FB error, status: %i", (int)status);
		return false;
	}

	return true;
}

void GBuffer::bindForWriting() {
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _fbo);
}

void GBuffer::bindForReading() {
	glBindFramebuffer(GL_READ_FRAMEBUFFER, _fbo);
}

void GBuffer::setReadBuffer(GBufferTextureType textureType) {
	glReadBuffer(GL_COLOR_ATTACHMENT0 + textureType);
}

}
