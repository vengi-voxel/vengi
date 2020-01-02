/**
 * @file
 */

#pragma once

#include "Types.h"

namespace video {

class FrameBuffer;

/**
 * @sa FrameBuffer
 * @ingroup Video
 */
class ScopedFrameBuffer {
private:
	Id _oldFramebuffer;
public:
	explicit ScopedFrameBuffer(const FrameBuffer& fbo);

	explicit ScopedFrameBuffer(Id bindHandle);

	~ScopedFrameBuffer();
};

}
