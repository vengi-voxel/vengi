/**
 * @file
 */

#include "FrameBuffer.h"

namespace video {

FrameBuffer::~FrameBuffer() {
	core_assert_msg(_fbo == video::InvalidId, "Framebuffer was not properly shut down");
	shutdown();
}

bool FrameBuffer::init(const glm::ivec2& dimension) {
	_dimension = dimension;
	return video::setupFramebuffer(_fbo, _texture, _depth, dimension);
}

void FrameBuffer::shutdown() {
	video::deleteFramebuffer(_fbo);
	video::deleteRenderbuffer(_depth);
	video::deleteTexture(_texture);
}

void FrameBuffer::bind() {
	video::getViewport(_viewport[0], _viewport[1], _viewport[2], _viewport[3]);
	video::viewport(0, 0, _dimension.x, _dimension.y);
	_oldFramebuffer = video::bindFramebuffer(FrameBufferMode::Default, _fbo, _texture);
	video::clear(ClearFlag::Color | ClearFlag::Depth);
}

void FrameBuffer::unbind() {
	video::viewport(_viewport[0], _viewport[1], _viewport[2], _viewport[3]);
	video::bindFramebuffer(FrameBufferMode::Draw, _oldFramebuffer);
	_oldFramebuffer = InvalidId;
}

}
