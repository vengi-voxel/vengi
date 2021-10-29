/**
 * @file
 */

#include "Renderer.h"

namespace video {

struct alignas(16) DrawArraysIndirectCommand {
	uint32_t count;				// to the number of used vertices
	uint32_t instanceCount = 1; // instances to draw of the current object
	uint32_t firstIndex = 0;	// the location of the first vertex relative the current object
	uint32_t baseInstance = 0;	// the first instance to be rendered
};

struct alignas(16) DrawElementsIndirectCommand {
	uint32_t count;				// to the number of used vertices
	uint32_t instanceCount = 1; // instances to draw of the current object
	uint32_t firstIndex = 0;	// the location of the first vertex relative the current object
	uint32_t baseVertex = 0;	// location of first vertex of the current object
	uint32_t baseInstance = 0;	// the first instance to be rendered
};

/**
 * @brief This buffer holds the draw commands for the indirect draw call
 * @sa Renderer::drawElementsIndirect()
 */
class IndirectDrawBuffer {
private:
	video::Id _handle = video::InvalidId;

public:
	bool init();
	void shutdown();

	bool update(const DrawElementsIndirectCommand* data, size_t size);
	bool bind() const;
	bool unbind() const;
};

}
