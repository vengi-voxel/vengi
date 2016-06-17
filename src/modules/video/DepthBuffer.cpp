/**
 * @file
 */

#include "DepthBuffer.h"
#include "GLFunc.h"
#include "ScopedFrameBuffer.h"

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

	if (_depthTexture != 0) {
		glDeleteTextures(1, &_depthTexture);
		_depthTexture = 0;
	}

	core_assert(_oldFramebuffer == -1);
}

bool DepthBuffer::init(int width, int height, bool antialiased) {
	_width = width;
	_height = height;

	glGenFramebuffers(1, &_fbo);
	GL_setName(GL_FRAMEBUFFER, _fbo, "depthfbo");
	ScopedFrameBuffer scopedFrameBuffer(_fbo);

	glGenTextures(1, &_depthTexture);
	GL_setName(GL_TEXTURE, _depthTexture, "depthtexture");
	glBindTexture(GL_TEXTURE_2D, _depthTexture);
	if (antialiased) {
		// GL_LINEAR because we want to have values between 0.0 and 1.0 in the shadowmap to get
		// anti-aliased shadow maps. You have to use sampler2DShadow in your shader to get PCF with this.
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	} else {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	GLfloat borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	if (antialiased) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	} else {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
	}
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _depthTexture, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	GL_checkError();

	const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		Log::error("FB error, status: %i", (int)status);
		return false;
	}

	return true;
}

void DepthBuffer::bind() {
	core_assert(_oldFramebuffer == -1);
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &_oldFramebuffer);
	GL_checkError();

	glGetIntegerv(GL_VIEWPORT, _viewport);
	glViewport(0, 0, _width, _height);
	glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
	//glClearDepth(0); // black
	//glClearDepth(1); // white
	glClear(GL_DEPTH_BUFFER_BIT);
	GL_checkError();
}

float *DepthBuffer::read() {
	ScopedFrameBuffer scopedFrameBuffer(_fbo);
	float *depths = new float[_width * _height];
	glReadPixels(0, 0, _width, _height, GL_DEPTH_COMPONENT, GL_FLOAT, depths);
	return depths;
}

void DepthBuffer::unbind() {
	glViewport(_viewport[0], _viewport[1], (GLsizei)_viewport[2], (GLsizei)_viewport[3]);
	core_assert(_oldFramebuffer != -1);
	glBindFramebuffer(GL_FRAMEBUFFER, _oldFramebuffer);
	_oldFramebuffer = -1;
}

}
