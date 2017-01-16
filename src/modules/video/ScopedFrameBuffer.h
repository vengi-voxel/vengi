#pragma once

#include "FrameBuffer.h"

namespace video {

class ScopedFrameBuffer {
public:
	explicit ScopedFrameBuffer(const FrameBuffer& fbo) :
			ScopedFrameBuffer(fbo._fbo) {
	}

	explicit ScopedFrameBuffer(Id bindHandle) {
		glBindFramebuffer(GL_FRAMEBUFFER, bindHandle);
		GL_checkError();
	}

	~ScopedFrameBuffer() {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
};

}
