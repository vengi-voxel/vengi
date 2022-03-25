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
	voxelformat::SceneGraph _sceneGraph;
	bool _sceneMode = true;

public:
	void construct() override;
	bool init() override;
	void update();
	void shutdown() override;

	void setAmbientColor(const glm::vec3& color);
	void setDiffuseColor(const glm::vec3& color);

	void setSceneMode(bool sceneMode);
	bool extractRegion(voxelformat::SceneGraphNode &node, const voxel::Region& region);
	void translate(voxelformat::SceneGraphNode &node, const glm::ivec3 &v);
	bool toMesh(voxelformat::SceneGraphNode &node, voxel::Mesh* mesh);
	bool empty(voxelformat::SceneGraphNode &node);
	void prepare(voxelformat::SceneGraph &sceneGraph, int frame = 0, bool hideInactive = false, bool grayInactive = false);
	/**
	 * @param waitPending Wait for pending extractions and update the buffers before doing the rendering. If this is false, you have to call @c update() manually!
	 */
	void render(const video::Camera& camera, bool shadow = true, bool waitPending = false);
	void clear();
};

inline void SceneGraphRenderer::setSceneMode(bool sceneMode) {
	_sceneMode = sceneMode;
}

} // namespace voxelrender
