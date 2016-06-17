/**
 * @file
 */

#include "FrameBuffer.h"

namespace video {

FrameBuffer::FrameBuffer() :
		_framebuffer(0), _attached(0) {
}

FrameBuffer::~FrameBuffer() {
	core_assert_msg(_framebuffer == 0u, "Framebuffer was not properly shut down");
	shutdown();
}

void FrameBuffer::shutdown() {
	if (_framebuffer != 0)
		glDeleteFramebuffers(1, &_framebuffer);
	_framebuffer = 0;
}

void FrameBuffer::bind() {
	if (_framebuffer != 0) {
		glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
		return;
	}

	glGenFramebuffers(1, &_framebuffer);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _framebuffer);
}

void FrameBuffer::unbind() {
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

bool FrameBuffer::isSuccessful() {
	GLenum error = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	return error == GL_FRAMEBUFFER_COMPLETE;
}

void FrameBuffer::attachRenderBuffer(GLenum internalformat, GLenum attachment, GLsizei width, GLsizei height) {
	// TODO: shutdown support for render buffers, too
	GLuint rbo;
	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, internalformat, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, rbo);
}

void FrameBuffer::attachTexture(GLuint texture, GLenum attachmentType) {
	if (texture != 0)
		++_attached;
	else
		--_attached;
	glBindTexture(GL_TEXTURE_2D, texture);
	glFramebufferTexture(GL_FRAMEBUFFER, attachmentType, texture, 0);
}

void FrameBuffer::drawBuffers(GLsizei n, const GLenum *buffers) {
	glDrawBuffers(n, buffers);
}

}
