/**
 * @file
 */

#pragma once

#include "FrameBuffer.h"

namespace video {

/**
 * @ingroup Video
 */
class ScopedFrameBuffer {
private:
	Id _oldFramebuffer;
public:
	explicit ScopedFrameBuffer(const FrameBuffer& fbo) :
			ScopedFrameBuffer(fbo._fbo) {
	}

	explicit ScopedFrameBuffer(Id bindHandle) {
		_oldFramebuffer = video::bindFramebuffer(video::FrameBufferMode::Default, bindHandle);
	}

	~ScopedFrameBuffer() {
		video::bindFramebuffer(video::FrameBufferMode::Default, _oldFramebuffer);
	}
};

}
