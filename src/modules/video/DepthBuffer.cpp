/**
 * @file
 */

#include "DepthBuffer.h"

#include <cstddef>
#include "core/Common.h"

namespace video {

DepthBuffer::DepthBuffer() {
}

DepthBuffer::~DepthBuffer() {
	shutdown();
}

void DepthBuffer::shutdown() {
	if (_fbo != 0) {
		glDeleteFramebuffers(1, &_fbo);
		_fbo = 0;
	}

	if (_depthTexture != 0) {
		glDeleteTextures(1, &_depthTexture);
		_depthTexture = 0;
	}
}

bool DepthBuffer::init(int width, int height) {
	_width = width;
	_height = height;
	glGenFramebuffers(1, &_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, _fbo);

	glGenTextures(1, &_depthTexture);

	glBindTexture(GL_TEXTURE_2D, _depthTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	GLfloat borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _depthTexture, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	GL_checkError();

	const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	// restore default FBO
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	GL_checkError();

	if (status != GL_FRAMEBUFFER_COMPLETE) {
		Log::error("FB error, status: %i", (int)status);
		return false;
	}

	return true;
}

void DepthBuffer::bind() {
	glGetIntegerv(GL_VIEWPORT, _viewport);
	glViewport(0, 0, _width, _height);
	glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
	//glClearDepth(0); // black
	//glClearDepth(1); // white
	glClear(GL_DEPTH_BUFFER_BIT);
	GL_checkError();
}

float *DepthBuffer::read() {
	glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
	float *depths = new float[_width * _height];
	glReadPixels(0, 0, _width, _height, GL_DEPTH_COMPONENT, GL_FLOAT, depths);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	return depths;
}

void DepthBuffer::unbind() {
	glViewport(_viewport[0], _viewport[1], (GLsizei)_viewport[2], (GLsizei)_viewport[3]);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

}
