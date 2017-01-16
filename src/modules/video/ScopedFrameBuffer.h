#pragma once

#include "FrameBuffer.h"

namespace video {

class ScopedFrameBuffer {
public:
	explicit ScopedFrameBuffer(const FrameBuffer& fbo) :
			ScopedFrameBuffer(fbo._fbo) {
	}

	explicit ScopedFrameBuffer(Id bindHandle) {
		video::bindFrameBuffer(video::FrameBufferMode::Default, bindHandle);
	}

	~ScopedFrameBuffer() {
		video::bindFrameBuffer(video::FrameBufferMode::Default, InvalidId);
	}
};

}
