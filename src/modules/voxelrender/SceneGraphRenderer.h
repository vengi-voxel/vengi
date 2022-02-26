/**
 * @file
 */

#pragma once

#include "RawVolumeRenderer.h"
#include "core/IComponent.h"
#include "voxelformat/SceneGraph.h"

/**
 * Basic voxel rendering
 */
namespace voxelrender {

/**
 * @brief Rendering of a voxel::SceneGraph
 */
class SceneGraphRenderer : public core::IComponent {
protected:
	RawVolumeRenderer _renderer;
	voxel::SceneGraph _sceneGraph;

public:
	void construct() override;
	bool init() override;
	void update();
	void shutdown() override;

	void setAmbientColor(const glm::vec3& color);
	void setDiffuseColor(const glm::vec3& color);

	bool extractRegion(voxel::SceneGraphNode &node, const voxel::Region& region);
	void translate(voxel::SceneGraphNode &node, const glm::ivec3 &v);
	bool toMesh(voxel::SceneGraphNode &node, voxel::Mesh* mesh);
	bool empty(voxel::SceneGraphNode &node);
	void prepare(voxel::SceneGraph &sceneGraph, bool hideInactive = false, bool grayInactive = false);
	/**
	 * @param waitPending Wait for pending extractions and update the buffers before doing the rendering. If this is false, you have to call @c update() manually!
	 */
	void render(const video::Camera& camera, bool shadow = true, bool waitPending = false);
	void clear();
};

} // namespace voxelrender
