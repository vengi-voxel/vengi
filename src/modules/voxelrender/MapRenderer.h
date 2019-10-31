/**
 * @file
 */

#pragma once

#include "voxelworld/WorldMgr.h"
#include "core/IComponent.h"
#include "video/Camera.h"
#include "video/FrameBuffer.h"

namespace voxelrender {

/**
 * @brief Class to render the minimap and the worldmap of the voxel chunks
 */
class MapRenderer : public core::IComponent {
private:
	voxelworld::WorldMgrPtr _world;
	video::FrameBuffer _frameBuffer;
public:
	MapRenderer(const voxelworld::WorldMgrPtr& world);

	bool init() override;
	void shutdown() override;

	// TODO: maybe the list of chunks to include in the minimap? That way we can also use this class
	// for the worldmap
	void updateMiniMap();

	int renderMiniMap(const video::Camera& camera, const glm::vec2& pos, int* vertices);
};

}
