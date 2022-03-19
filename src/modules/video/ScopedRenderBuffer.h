/**
 * @file
 */

#pragma once

#include "Types.h"

namespace video {

class RenderBuffer;

/**
 * @sa RenderBuffer
 * @ingroup Video
 */
class ScopedRenderBuffer {
private:
	Id _oldRenderbuffer;
public:
	explicit ScopedRenderBuffer(const RenderBuffer& rbo);

	explicit ScopedRenderBuffer(Id bindHandle);

	~ScopedRenderBuffer();
};

}
