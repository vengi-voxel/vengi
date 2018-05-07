/**
 * @file
 */

#pragma once

#include "FrameBuffer.h"

namespace video {

/**
 * @sa FrameBuffer
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
		_oldFramebuffer = video::bindFramebuffer(bindHandle);
	}

	~ScopedFrameBuffer() {
		video::bindFramebuffer(_oldFramebuffer);
	}
};

}
