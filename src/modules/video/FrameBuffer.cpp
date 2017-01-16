/**
 * @file
 */

#include "FrameBuffer.h"
#include "ScopedFrameBuffer.h"
#include "Renderer.h"

namespace video {

FrameBuffer::FrameBuffer() {
}

FrameBuffer::~FrameBuffer() {
	core_assert_msg(_fbo == video::InvalidId, "Framebuffer was not properly shut down");
	shutdown();
}

bool FrameBuffer::init(const glm::ivec2& dimension) {
	_dimension = dimension;

	_fbo = video::genFramebuffer();
	ScopedFrameBuffer scopedFrameBuffer(_fbo);

	_texture = video::genTexture();
	video::bindTexture(video::TextureUnit::Zero, TextureType::Texture2D, _texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, dimension.x, dimension.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _texture, 0);

	_depth = video::genRenderbuffer();
	video::bindRenderbuffer(_depth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, dimension.x, dimension.y);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depth);

	video::checkError();

	const GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, drawBuffers);

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

void FrameBuffer::shutdown() {
	video::deleteFramebuffer(_fbo);
	video::deleteRenderbuffer(_depth);
	video::deleteTexture(_texture);
}

void FrameBuffer::bind(bool read) {
	video::getViewport(_viewport[0], _viewport[1], _viewport[2], _viewport[3]);
	video::viewport(0, 0, _dimension.x, _dimension.y);
	video::bindFramebuffer(FrameBufferMode::Default, _fbo);
	if (!read) {
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _texture, 0);
	}
	video::clear(ClearFlag::Color | ClearFlag::Depth);
}

void FrameBuffer::unbind() {
	video::viewport(_viewport[0], _viewport[1], _viewport[2], _viewport[3]);
	video::bindFramebuffer(FrameBufferMode::Draw, InvalidId);
}

}
