#pragma once

#include "GLFunc.h"

namespace video {

class ScopedFrameBuffer {
private:
	GLint _handle;
public:
	ScopedFrameBuffer() {
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &_handle);
		GL_checkError();
	}

	explicit ScopedFrameBuffer(GLint bindHandle) {
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &_handle);
		GL_checkError();
		glBindFramebuffer(GL_FRAMEBUFFER, bindHandle);
		GL_checkError();
	}

	~ScopedFrameBuffer() {
		glBindFramebuffer(GL_FRAMEBUFFER, _handle);
		GL_checkError();
	}
};

}
