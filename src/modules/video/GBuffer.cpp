/**
 * @file
 */

#include "GBuffer.h"
#include "ScopedFrameBuffer.h"

#include <cstddef>
#include "core/Common.h"

namespace video {

GBuffer::GBuffer() :
		_fbo(InvalidId), _depthTexture(InvalidId) {
	for (std::size_t i = 0; i < SDL_arraysize(_textures); ++i) {
		_textures[i] = InvalidId;
	}
}

GBuffer::~GBuffer() {
	core_assert_msg(_fbo == InvalidId, "GBuffer was not properly shut down");
	shutdown();
}

void GBuffer::shutdown() {
	if (_fbo != InvalidId) {
		glDeleteFramebuffers(1, &_fbo);
		_fbo = InvalidId;
	}

	if (_textures[0] != InvalidId) {
		const int texCount = (int)SDL_arraysize(_textures);
		glDeleteTextures(texCount, _textures);
		for (int i = 0; i < texCount; ++i) {
			_textures[i] = InvalidId;
		}
	}

	if (_depthTexture != InvalidId) {
		glDeleteTextures(1, &_depthTexture);
		_depthTexture = InvalidId;
	}
	core_assert(_oldDrawFramebuffer == InvalidId);
	core_assert(_oldReadFramebuffer == InvalidId);
}

bool GBuffer::init(const glm::ivec2& dimension) {
	glGenFramebuffers(1, &_fbo);
	ScopedFrameBuffer scopedFrameBuffer(_fbo);

	// +1 for the depth texture
	glGenTextures(SDL_arraysize(_textures) + 1, _textures);
	for (std::size_t i = 0; i < SDL_arraysize(_textures); ++i) {
		glBindTexture(GL_TEXTURE_2D, _textures[i]);
		// we are going to write vec3 into the out vars in the shaders
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, dimension.x, dimension.y, 0, GL_RGB, GL_FLOAT, nullptr);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, _textures[i], 0);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		video::checkError();
	}

	glBindTexture(GL_TEXTURE_2D, _depthTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, dimension.x, dimension.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _depthTexture, 0);
	video::checkError();

	const GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	static_assert(SDL_arraysize(drawBuffers) == SDL_arraysize(_textures), "buffers and textures don't match");
	glDrawBuffers(SDL_arraysize(drawBuffers), drawBuffers);
	video::checkError();

	const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		Log::error("FB error, status: %i", (int)status);
		return false;
	}

	return true;
}

void GBuffer::bindForWriting() {
	if (_oldDrawFramebuffer == InvalidId) {
		glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, (GLint*)&_oldDrawFramebuffer);
		video::checkError();
	}

	video::bindFrameBuffer(FrameBufferMode::Draw, _fbo);
}

void GBuffer::bindForReading(bool gbuffer) {
	if (gbuffer) {
		if (_oldReadFramebuffer == InvalidId) {
			glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, (GLint*)&_oldReadFramebuffer);
			video::checkError();
		}
		bindFrameBuffer(FrameBufferMode::Read, _fbo);
		return;
	}

	if (_oldDrawFramebuffer == InvalidId) {
		glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, (GLint*)&_oldDrawFramebuffer);
		video::checkError();
	}
	bindFrameBuffer(FrameBufferMode::Draw, InvalidId);

	// activate the textures to read from
	for (int i = 0; i < (int) SDL_arraysize(_textures); ++i) {
		glActiveTexture(GL_TEXTURE0 + i);
		core_assert(_textures[i] != 0);
		glBindTexture(GL_TEXTURE_2D, _textures[i]);
	}
	glActiveTexture(GL_TEXTURE0);
}

void GBuffer::unbind() {
	if (_oldDrawFramebuffer != InvalidId) {
		bindFrameBuffer(FrameBufferMode::Draw, (Id)_oldDrawFramebuffer);
		_oldDrawFramebuffer = InvalidId;
	}
	if (_oldReadFramebuffer != InvalidId) {
		bindFrameBuffer(FrameBufferMode::Read, (Id)_oldReadFramebuffer);
		_oldReadFramebuffer = InvalidId;
	}
}

void GBuffer::setReadBuffer(GBufferTextureType textureType) {
	glReadBuffer(GL_COLOR_ATTACHMENT0 + textureType);
	video::checkError();
}

}
