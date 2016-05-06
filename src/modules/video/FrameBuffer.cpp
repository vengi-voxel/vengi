/**
 * @file
 */

#include "FrameBuffer.h"

namespace video {

FrameBuffer::FrameBuffer() :
		_framebuffer(0), _attached(0) {
}

FrameBuffer::~FrameBuffer() {
	if (_framebuffer != 0)
		glDeleteFramebuffers(1, &_framebuffer);
}

void FrameBuffer::bind() {
	if (_framebuffer != 0) {
		glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
		return;
	}

	glGenFramebuffers(1, &_framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
}

void FrameBuffer::unbind() {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

bool FrameBuffer::isSuccessful() {
	GLenum error = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	return error == GL_FRAMEBUFFER_COMPLETE;
}

void FrameBuffer::attachRenderBuffer(GLenum internalformat, GLenum attachment, GLsizei width, GLsizei height) {
	GLuint depthrenderbuffer;
	glGenRenderbuffers(1, &depthrenderbuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, depthrenderbuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, internalformat, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, depthrenderbuffer);
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
