/**
 * @file
 */

#include "ScopedFrameBuffer.h"
#include "FrameBuffer.h"
#include "Renderer.h"

namespace video {

ScopedFrameBuffer::ScopedFrameBuffer(const FrameBuffer& fbo) :
		ScopedFrameBuffer(fbo._fbo) {
}

ScopedFrameBuffer::ScopedFrameBuffer(Id bindHandle) {
	_oldFramebuffer = video::bindFramebuffer(bindHandle);
}

ScopedFrameBuffer::~ScopedFrameBuffer() {
	video::bindFramebuffer(_oldFramebuffer);
}

}
