/**
 * @file
 */

#include "Renderer.h"
#include "RenderBuffer.h"
#include "core/Log.h"
#include "core/Assert.h"

namespace video {

RenderBuffer::RenderBuffer(TextureFormat format, int w, int h, int samples) :
		_format(format), _w(w), _h(h), _samples(samples) {
}

RenderBuffer::~RenderBuffer() {
	core_assert_msg(_rbo == video::InvalidId, "RenderBuffer was not properly shut down");
	shutdown();
}

bool RenderBuffer::init() {
	video::genRenderbuffers(1, &_rbo);
	const Id old = video::bindRenderbuffer(_rbo);
	video::setupRenderBuffer(_format, _w, _h, _samples);
	video::bindRenderbuffer(old);
	return true;
}

void RenderBuffer::shutdown() {
	video::deleteRenderbuffer(_rbo);
}

RenderBufferPtr createRenderBuffer(TextureFormat format, int w, int h, int samples) {
	const RenderBufferPtr& ptr = std::make_shared<RenderBuffer>(format, w, h, samples);
	if (!ptr->init()) {
		Log::warn("Could not init renderbuffer");
		return RenderBufferPtr();
	}
	return ptr;
}

}
