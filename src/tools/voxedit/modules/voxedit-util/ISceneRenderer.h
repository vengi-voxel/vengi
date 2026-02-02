/**
 * @file
 */

#pragma once

#include "core/IComponent.h"
#include "math/Axis.h"
#include "scenegraph/SceneGraphNode.h"
#include "video/Camera.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxelrender/RenderContext.h"

namespace scenegraph {
class SceneGraph;
}

namespace voxedit {

/**
 * @brief Interface for the scene render component
 *
 * This mainly exists because the tests that run in headless mode, don't always have a graphical context available.
 *
 * @sa SceneRenderer
 */
class ISceneRenderer : public core::IComponent {
public:
	virtual ~ISceneRenderer() = default;

	struct RendererStats {
		int pendingExtractions = 0;
		int pendingMeshes = 0;
		int culledVolumes = 0;
	};

	virtual void update() {
	}
	virtual void clear() {
	}
	bool init() override {
		return true;
	}
	void shutdown() override {
	}
	virtual void updateLockedPlanes(math::Axis lockedAxis, const scenegraph::SceneGraph &sceneGraph,
									const glm::ivec3 &cursorPosition) {
	}
	virtual void updateNodeRegion(int nodeId, const voxel::Region &region, uint64_t renderRegionMillis = 0) {
	}
	virtual void updateGridRegion(const voxel::Region &region) {
	}
	virtual bool isVisible(int nodeId, bool hideEmpty = true) const {
		return true;
	}
	virtual void removeNode(int nodeId) {
	}
	virtual void renderUI(voxelrender::RenderContext &renderContext, const video::Camera &camera) {
	}
	virtual void renderScene(voxelrender::RenderContext &renderContext, const video::Camera &camera) {
	}
	virtual const voxel::RawVolume *volumeForNode(const scenegraph::SceneGraphNode &node) {
		return node.volume();
	}
	virtual const voxel::Region &sliceRegion() const {
		return voxel::Region::InvalidRegion;
	}
	virtual void setSliceRegion(const voxel::Region &region) {
	}
	virtual bool isSliceModeActive() const {
		return sliceRegion().isValid();
	}
	virtual RendererStats rendererStats() const {
		return {};
	}
};

using SceneRendererPtr = core::SharedPtr<ISceneRenderer>;

} // namespace voxedit
