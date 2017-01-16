#pragma once

#include "FrameBuffer.h"

namespace video {

class ScopedFrameBuffer {
public:
	explicit ScopedFrameBuffer(const FrameBuffer& fbo) :
			ScopedFrameBuffer(fbo._fbo) {
	}

	explicit ScopedFrameBuffer(Id bindHandle) {
		video::bindFramebuffer(video::FrameBufferMode::Default, bindHandle);
	}

	~ScopedFrameBuffer() {
		video::bindFramebuffer(video::FrameBufferMode::Default, InvalidId);
	}
};

}
