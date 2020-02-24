/**
 * @file
 */

#pragma once

#include "Types.h"
#include <memory>

namespace video {

class RenderBuffer;
typedef std::shared_ptr<RenderBuffer> RenderBufferPtr;

class RenderBuffer {
private:
	Id _rbo = video::InvalidId;
	const TextureFormat _format;
	int _w;
	int _h;
	int _samples;
public:
	RenderBuffer(TextureFormat format, int w, int h, int samples = 0);
	~RenderBuffer();

	bool init();
	void shutdown();

	Id handle() const;
};

inline Id RenderBuffer::handle() const {
	return _rbo;
}

extern RenderBufferPtr createRenderBuffer(TextureFormat format, int w, int h, int samples = 0);

}
