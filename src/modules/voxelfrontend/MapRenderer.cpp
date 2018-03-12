/**
 * @file
 */

#include "MapRenderer.h"
#include "video/ScopedFrameBuffer.h"

namespace frontend {

MapRenderer::MapRenderer(const voxel::WorldPtr& world) :
		_world(world) {
}

void MapRenderer::shutdown() {
	_frameBuffer.shutdown();
}

bool MapRenderer::init() {
	// TODO: size
	const glm::vec2 dim(42, 42);
	if (!_frameBuffer.init(dim)) {
		return false;
	}
	// TODO: setup shader
	return true;
}

void MapRenderer::updateMiniMap() {
	// TODO: render minimap to texture
	video::ScopedFrameBuffer scoped(_frameBuffer);
}

int MapRenderer::renderMiniMap(const video::Camera& camera, const glm::vec2& pos, int* vertices) {
	// TODO: render fbo in ortho mode to the given coordinates
	return 0;
}

}
