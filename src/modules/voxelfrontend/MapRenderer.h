/**
 * @file
 */

#pragma once

#include "voxel/World.h"
#include "core/IComponent.h"
#include "video/Camera.h"
#include "video/FrameBuffer.h"
#include "FrontendShaders.h"

namespace frontend {

/**
 * @brief Class to render the minimap and the worldmap of the voxel chunks
 */
class MapRenderer : public core::IComponent {
private:
	voxel::WorldPtr _world;
	video::FrameBuffer _frameBuffer;
public:
	MapRenderer(const voxel::WorldPtr& world);

	bool init() override;
	void shutdown() override;

	// TODO: maybe the list of chunks to include in the minimap? That way we can also use this class
	// for the worldmap
	void updateMiniMap();

	int renderMiniMap(const video::Camera& camera, const glm::vec2& pos, int* vertices);
};

}
