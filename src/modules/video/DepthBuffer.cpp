/**
 * @file
 */

#include "DepthBuffer.h"
#include "Renderer.h"
#include "ScopedFrameBuffer.h"
#include "Texture.h"

#include <cstddef>
#include "core/Common.h"

namespace video {

DepthBuffer::DepthBuffer() :
		_depthTexture(TextureType::Texture2DArray, TextureFormat::D24S8, "**depthmap**", 1, 1, 1, TextureWrap::ClampToEdge) {
}

DepthBuffer::~DepthBuffer() {
	core_assert_msg(_fbo == InvalidId, "Depthbuffer was not properly shut down");
	shutdown();
}

void DepthBuffer::shutdown() {
	if (_fbo != InvalidId) {
		glDeleteFramebuffers(1, &_fbo);
		_fbo = InvalidId;
	}

	_depthTexture.shutdown();

	if (_rbo != InvalidId) {
		glDeleteRenderbuffers(1, &_rbo);
		_rbo = InvalidId;
	}
}

bool DepthBuffer::init(const glm::ivec2& dimension, DepthBufferMode mode, int textureCount) {
	if (textureCount > 4 || textureCount < 1) {
		Log::error("Invalid texture count for depthbuffer: %i", textureCount);
		return false;
	}
	_mode = mode;

	TextureFormat format;
	if (depthAttachment()) {
		format = TextureFormat::D24S8;
	} else {
		format = TextureFormat::RGBA;
	}
	_depthTexture.upload(format, dimension.x, dimension.y, nullptr, textureCount);
	if (depthCompare()) {
		video::bindTexture(TextureUnit::Upload, _depthTexture);
		const TextureType type = textureType();
		const GLenum glType = std::enum_value(type);
		glTexParameteri(glType, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
		glTexParameteri(glType, GL_TEXTURE_COMPARE_FUNC, GL_LESS);
	}

	glGenFramebuffers(1, &_fbo);
	ScopedFrameBuffer scopedFrameBuffer(_fbo);
	if (depthAttachment()) {
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
	}
	video::checkError();

	if (!depthAttachment()) {
		const GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(SDL_arraysize(drawBuffers), drawBuffers);
	}
	return true;
}

bool DepthBuffer::bind() {
	glGetIntegerv(GL_VIEWPORT, _oldViewport);
	glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
	glViewport(0, 0, _depthTexture.width(), _depthTexture.height());
	video::checkError();
	return true;
}

bool DepthBuffer::bindTexture(int textureIndex) {
	core_assert(textureIndex >= 0 && textureIndex < 4);
	if (textureIndex < 0 || textureIndex >= 4) {
		return false;
	}
	if (depthAttachment()) {
		glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, _depthTexture, 0, textureIndex);
		video::clear(video::ClearFlag::Depth);
	} else {
		glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, _depthTexture, 0, textureIndex);
		video::clear(video::ClearFlag::Color | video::ClearFlag::Depth);
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

	video::checkError();
	return true;
}

void DepthBuffer::unbind() {
	glViewport(_oldViewport[0], _oldViewport[1], (GLsizei)_oldViewport[2], (GLsizei)_oldViewport[3]);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

}
