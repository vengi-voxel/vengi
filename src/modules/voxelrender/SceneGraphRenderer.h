/**
 * @file
 */

#pragma once

#include "RawVolumeRenderer.h"
#include "core/collection/DynamicArray.h"
#include "render/CameraFrustum.h"
#include "voxelformat/SceneGraph.h"

/**
 * Basic voxel rendering
 */
namespace voxelrender {

/**
 * @brief Rendering of a voxel::SceneGraph
 */
class SceneGraphRenderer : public core::NonCopyable {
protected:
	RawVolumeRenderer _renderer;
	render::CameraFrustum _cameraRenderer;
	core::DynamicArray<video::Camera> _cameras;
	bool _sceneMode = true;

public:
	void construct();
	bool init(const glm::ivec2 &size);
	void update();
	void shutdown();

	bool resize(const glm::ivec2 &size);
	void setAmbientColor(const glm::vec3& color);
	void setDiffuseColor(const glm::vec3& color);

	void setSceneMode(bool sceneMode);
	bool extractRegion(voxelformat::SceneGraphNode &node, const voxel::Region& region);
	bool toMesh(voxelformat::SceneGraphNode &node, voxel::Mesh* mesh);
	bool empty(voxelformat::SceneGraphNode &node);
	void prepare(voxelformat::SceneGraph &sceneGraph, voxelformat::FrameIndex frame = 0, bool hideInactive = false, bool grayInactive = false);
	void extractAll();
	/**
	 * @param waitPending Wait for pending extractions and update the buffers before doing the rendering. If this is false, you have to call @c update() manually!
	 */
	void render(const video::Camera& camera, bool shadow = true, bool waitPending = false);
	void clear();
};

inline void SceneGraphRenderer::setSceneMode(bool sceneMode) {
	_sceneMode = sceneMode;
}

inline bool SceneGraphRenderer::resize(const glm::ivec2 &size) {
	return _renderer.resize(size);
}

} // namespace voxelrender
