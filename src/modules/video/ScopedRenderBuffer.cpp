/**
 * @file
 */

#include "ScopedRenderBuffer.h"
#include "RenderBuffer.h"
#include "Renderer.h"

namespace video {

ScopedRenderBuffer::ScopedRenderBuffer(const RenderBuffer& rbo) :
		ScopedRenderBuffer(rbo.handle()) {
}

ScopedRenderBuffer::ScopedRenderBuffer(Id bindHandle) {
	_oldRenderbuffer = video::bindRenderbuffer(bindHandle);
}

ScopedRenderBuffer::~ScopedRenderBuffer() {
	video::bindRenderbuffer(_oldRenderbuffer);
}

}
