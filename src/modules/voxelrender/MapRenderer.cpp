/**
 * @file
 */

#include "MapRenderer.h"
#include "video/ScopedFrameBuffer.h"

namespace voxelrender {

MapRenderer::MapRenderer(const voxel::WorldMgrPtr& world) :
		_world(world) {
}

void MapRenderer::shutdown() {
	_frameBuffer.shutdown();
}

bool MapRenderer::init() {
	// TODO: size
	const glm::vec2 dim(42, 42);
	video::TextureConfig textureCfg;
	textureCfg.wrap(video::TextureWrap::ClampToEdge);
	video::FrameBufferConfig cfg;
	cfg.dimension(dim).depthBuffer(true).depthBufferFormat(video::TextureFormat::D24).addTextureAttachment(textureCfg);
	if (!_frameBuffer.init(cfg)) {
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
