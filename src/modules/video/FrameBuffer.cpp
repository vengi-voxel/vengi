/**
 * @file
 */

#include "FrameBuffer.h"
#include "ScopedFrameBuffer.h"
#include "GLFunc.h"

namespace video {

FrameBuffer::FrameBuffer() {
}

FrameBuffer::~FrameBuffer() {
	core_assert_msg(_fbo == 0u, "Framebuffer was not properly shut down");
	shutdown();
}

bool FrameBuffer::init(const glm::ivec2& dimension) {
	_dimension = dimension;

	glGenFramebuffers(1, &_fbo);
	ScopedFrameBuffer scopedFrameBuffer(_fbo);

	glGenTextures(1, &_texture);
	glBindTexture(GL_TEXTURE_2D, _texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, dimension.x, dimension.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glBindTexture(GL_TEXTURE_2D, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _texture, 0);
	GL_checkError();

	const GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, drawBuffers);

	const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		Log::error("FB error, status: %i", (int)status);
		return false;
	}

	return true;
}

void FrameBuffer::shutdown() {
	if (_fbo != 0) {
		glDeleteFramebuffers(1, &_fbo);
	}
	if (_texture != 0) {
		glDeleteTextures(1, &_texture);
		_texture = 0;
	}
	_fbo = 0;
	core_assert(_oldFramebuffer == -1);
}

void FrameBuffer::bind(bool read) {
	core_assert(_oldFramebuffer == -1);
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &_oldFramebuffer);
	GL_checkError();

	glGetIntegerv(GL_VIEWPORT, _viewport);
	glViewport(0, 0, _dimension.x, _dimension.y);
	glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
	if (!read) {
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _texture, 0);
	}
	glClear(GL_COLOR_BUFFER_BIT);
}

void FrameBuffer::unbind() {
	glViewport(_viewport[0], _viewport[1], (GLsizei)_viewport[2], (GLsizei)_viewport[3]);
	core_assert(_oldFramebuffer != -1);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _oldFramebuffer);
	_oldFramebuffer = -1;
}

}
