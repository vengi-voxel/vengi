/**
 * @file
 */

#include "Renderer.h"

namespace video {

class IndirectDrawBuffer {
private:
	video::Id _handle = video::InvalidId;

public:
	bool init();
	void shutdown();

	bool update(const void* data, size_t size);
	void bind() const;
	void unbind() const;
};

}