/**
 * @file
 */

#include "DepthBuffer.h"
#include "GLFunc.h"
#include "ScopedFrameBuffer.h"
#include "Texture.h"

#include <cstddef>
#include "core/Common.h"

namespace video {

DepthBuffer::DepthBuffer() {
}

DepthBuffer::~DepthBuffer() {
	core_assert_msg(_fbo == 0u, "Depthbuffer was not properly shut down");
	shutdown();
}

void DepthBuffer::shutdown() {
	if (_fbo != 0) {
		glDeleteFramebuffers(1, &_fbo);
		_fbo = 0;
	}

	for (int i = 0; i < (int)SDL_arraysize(_depthTexture); ++i) {
		if (_depthTexture[i] != 0) {
			glDeleteTextures(1, &_depthTexture[i]);
			_depthTexture[i] = 0;
		}
	}

	if (_rbo != 0) {
		glDeleteRenderbuffers(1, &_rbo);
		_rbo = 0;
	}

	core_assert(_oldFramebuffer == -1);
}

bool DepthBuffer::init(const glm::ivec2& dimension, DepthBufferMode mode, int textureCount) {
	if (textureCount > 4) {
		return false;
	}
	_dimension = dimension;
	_mode = mode;

	glGenFramebuffers(1, &_fbo);
	GL_setName(GL_FRAMEBUFFER, _fbo, "depthfbo");
	ScopedFrameBuffer scopedFrameBuffer(_fbo);

	const TextureType type = textureType();
	const GLenum glType = std::enum_value(type);
	glGenTextures(textureCount, _depthTexture);
	for (int i = 0; i < textureCount; ++i) {
		GL_setName(GL_TEXTURE, _depthTexture, "depthtexture");
		glBindTexture(glType, _depthTexture[i]);
		glTexParameteri(glType, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(glType, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(glType, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(glType, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(glType, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(glType, GL_TEXTURE_MAX_LEVEL, 0);
		if (depthAttachment()) {
			if (_mode == DepthBufferMode::DEPTH_CMP) {
				glTexParameteri(glType, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
				glTexParameteri(glType, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
			}
			glTexImage2D(glType, 0, GL_DEPTH_COMPONENT, dimension.x, dimension.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
		} else {
			glTexImage2D(glType, 0, GL_RGBA8, dimension.x, dimension.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		}
	}
	glBindTexture(GL_TEXTURE_2D, 0);
	if (depthAttachment()) {
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _depthTexture[0], 0);
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
	} else {
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _depthTexture[0], 0);
	}
	GL_checkError();

	if (!depthAttachment()) {
#if 0
		glGenRenderbuffers(1, &_rbo);
		glBindRenderbuffer(GL_RENDERBUFFER, _rbo);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, dimension.x, dimension.y);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _rbo);
		//glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, _rbo);
#endif

		const GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(SDL_arraysize(drawBuffers), drawBuffers);
	}

	const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		switch (status) {
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
			Log::error("FB error, incomplete attachment");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
			Log::error("FB error, incomplete missing attachment");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
			Log::error("FB error, incomplete draw buffer");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
			Log::error("FB error, incomplete read buffer");
			break;
		case GL_FRAMEBUFFER_UNSUPPORTED:
			Log::error("FB error, framebuffer unsupported");
			break;
		default:
			Log::error("FB error, status: %i", (int)status);
			break;
		}
		return false;
	}

	return true;
}

bool DepthBuffer::bind() {
	core_assert(_oldFramebuffer == -1);
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &_oldFramebuffer);
	GL_checkError();

	glGetIntegerv(GL_VIEWPORT, _viewport);
	glViewport(0, 0, _dimension.x, _dimension.y);
	glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
	return true;
}

bool DepthBuffer::bindTexture(bool read, int textureIndex) {
	core_assert(textureIndex >= 0);
	core_assert(textureIndex < (int)SDL_arraysize(_depthTexture));
	if (textureIndex < 0 || textureIndex >= (int)SDL_arraysize(_depthTexture)) {
		return false;
	}
	if (depthAttachment()) {
		if (!read) {
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _depthTexture[textureIndex], 0);
		}
		glClear(GL_DEPTH_BUFFER_BIT);
	} else {
		if (!read) {
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _depthTexture[textureIndex], 0);
		}
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
	GL_checkError();
	return true;
}

uint8_t *DepthBuffer::read() {
	if (depthAttachment()) {
		return nullptr;
	}
	ScopedFrameBuffer scopedFrameBuffer(_fbo);
	uint8_t *depths = new uint8_t[_dimension.x * _dimension.y * 4];
	glReadPixels(0, 0, _dimension.x, _dimension.y, GL_RGBA, GL_UNSIGNED_BYTE, depths);
	return depths;
}

void DepthBuffer::unbind() {
	glViewport(_viewport[0], _viewport[1], (GLsizei)_viewport[2], (GLsizei)_viewport[3]);
	core_assert(_oldFramebuffer != -1);
	glBindFramebuffer(GL_FRAMEBUFFER, _oldFramebuffer);
	_oldFramebuffer = -1;
}

}
