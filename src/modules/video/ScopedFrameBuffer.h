#pragma once

#include "FrameBuffer.h"

namespace video {

class ScopedFrameBuffer {
private:
	GLint _handle;
public:
	ScopedFrameBuffer() {
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &_handle);
		GL_checkError();
	}

	explicit ScopedFrameBuffer(const FrameBuffer& fbo) :
			ScopedFrameBuffer(fbo._fbo) {
	}

	explicit ScopedFrameBuffer(GLint bindHandle) {
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &_handle);
		GL_checkError();
		// check for double bind
		if (_handle == bindHandle) {
			_handle = -1;
			return;
		}
		glBindFramebuffer(GL_FRAMEBUFFER, bindHandle);
		GL_checkError();
	}

	~ScopedFrameBuffer() {
		if (_handle != -1) {
			glBindFramebuffer(GL_FRAMEBUFFER, _handle);
			GL_checkError();
		}
	}
};

}
