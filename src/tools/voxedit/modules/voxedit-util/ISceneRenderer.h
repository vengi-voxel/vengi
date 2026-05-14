/**
 * @file
 */

#pragma once

#include "core/IComponent.h"
#include "core/Trace.h"
#include "core/concurrent/Lock.h"
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
protected:
	enum class CommandType : uint8_t { NodeRegion, GridRegion, SliceRegion, RemoveNode, UnhideNode, Clear, MarkDirty, LockedPlanes };

	struct CommandEvent {
		CommandType type;
		union {
			struct {
				int nodeId;
				int32_t regionMins[3];
				int32_t regionMaxs[3];
				uint64_t renderRegionMillis;
			} nodeRegion;

			struct {
				int32_t regionMins[3];
				int32_t regionMaxs[3];
			} gridRegion;

			struct {
				int32_t regionMins[3];
				int32_t regionMaxs[3];
			} sliceRegion;

			struct {
				int nodeId;
			} node;

			struct {
				math::Axis lockedAxis;
				int32_t cursorPosition[3];
			} lockedPlanes;
		};
	};

	core_trace_mutex(core::Lock, _commandBufferMutex, "ISceneRenderer");
	core::DynamicArray<CommandEvent> _commandBuffer;

public:
	virtual ~ISceneRenderer() = default;

	struct RendererStats {
		int pendingExtractions = 0;
		int pendingMeshes = 0;
		int culledVolumes = 0;
		int freeVolumeIndices = 0;
		int nextVolumeIdx = 0;
		int mappedNodes = 0;
	};

	virtual void update() {
	}

	bool init() override {
		return true;
	}

	void shutdown() override {
	}

	virtual bool isVisible(int nodeId, bool hideEmpty = true) const {
		return true;
	}

	virtual void renderUI(voxelrender::RenderContext &renderContext, const video::Camera &camera) {
	}

	virtual void renderScene(voxelrender::RenderContext &renderContext, const video::Camera &camera) {
	}

	/**
	 * @brief Return the volume of a node that is registered in the renderer - this could theoretically differ from the
	 * node's own volume if only parts of the full volume is rendered (like e.g. 2d slices)
	 */
	virtual const voxel::RawVolume *volumeForNode(const scenegraph::SceneGraphNode &node) {
		return node.volume();
	}

	virtual const voxel::Region &sliceRegion() const {
		return voxel::Region::InvalidRegion;
	}

	virtual bool isSliceModeActive() const {
		return sliceRegion().isValid();
	}

	virtual RendererStats rendererStats() const {
		return {};
	}

	virtual void updateLockedPlanes(math::Axis lockedAxis, const glm::ivec3 &cursorPosition) {
		CommandEvent cmd;
		cmd.type = CommandType::LockedPlanes;
		cmd.lockedPlanes.lockedAxis = lockedAxis;
		cmd.lockedPlanes.cursorPosition[0] = cursorPosition.x;
		cmd.lockedPlanes.cursorPosition[1] = cursorPosition.y;
		cmd.lockedPlanes.cursorPosition[2] = cursorPosition.z;
		core::ScopedLock lock(_commandBufferMutex);
		_commandBuffer.push_back(cmd);
	}

	virtual void updateGridRegion(const voxel::Region &region) {
		CommandEvent cmd;
		cmd.type = CommandType::GridRegion;
		cmd.gridRegion.regionMins[0] = region.getLowerX();
		cmd.gridRegion.regionMins[1] = region.getLowerY();
		cmd.gridRegion.regionMins[2] = region.getLowerZ();
		cmd.gridRegion.regionMaxs[0] = region.getUpperX();
		cmd.gridRegion.regionMaxs[1] = region.getUpperY();
		cmd.gridRegion.regionMaxs[2] = region.getUpperZ();
		core::ScopedLock lock(_commandBufferMutex);
		_commandBuffer.push_back(cmd);
	}

	virtual void updateNodeRegion(int nodeId, const voxel::Region &region, uint64_t renderRegionMillis = 0) {
		CommandEvent cmd;
		cmd.type = CommandType::NodeRegion;
		cmd.nodeRegion.nodeId = nodeId;
		cmd.nodeRegion.regionMins[0] = region.getLowerX();
		cmd.nodeRegion.regionMins[1] = region.getLowerY();
		cmd.nodeRegion.regionMins[2] = region.getLowerZ();
		cmd.nodeRegion.regionMaxs[0] = region.getUpperX();
		cmd.nodeRegion.regionMaxs[1] = region.getUpperY();
		cmd.nodeRegion.regionMaxs[2] = region.getUpperZ();
		cmd.nodeRegion.renderRegionMillis = renderRegionMillis;
		core::ScopedLock lock(_commandBufferMutex);
		_commandBuffer.push_back(cmd);
	}

	virtual void removeNode(int nodeId) {
		CommandEvent cmd;
		cmd.type = CommandType::RemoveNode;
		cmd.node.nodeId = nodeId;
		core::ScopedLock lock(_commandBufferMutex);
		_commandBuffer.push_back(cmd);
	}

	/**
	 * @brief Mark scene-shape caches (AABB, bone meshes) as needing a rebuild
	 * @note Call whenever node visibility, active node, or scene structure changes
	 */
	virtual void markDirty() {
		CommandEvent cmd;
		cmd.type = CommandType::MarkDirty;
		core::ScopedLock lock(_commandBufferMutex);
		_commandBuffer.push_back(cmd);
	}

	virtual void unhideNode(int nodeId) {
		CommandEvent cmd;
		cmd.type = CommandType::UnhideNode;
		cmd.node.nodeId = nodeId;
		core::ScopedLock lock(_commandBufferMutex);
		_commandBuffer.push_back(cmd);
	}

	virtual void clear() {
		CommandEvent cmd;
		cmd.type = CommandType::Clear;
		core::ScopedLock lock(_commandBufferMutex);
		_commandBuffer.push_back(cmd);
	}

	virtual void setSliceRegion(const voxel::Region &region) {
		CommandEvent cmd;
		cmd.type = CommandType::SliceRegion;
		cmd.sliceRegion.regionMins[0] = region.getLowerX();
		cmd.sliceRegion.regionMins[1] = region.getLowerY();
		cmd.sliceRegion.regionMins[2] = region.getLowerZ();
		cmd.sliceRegion.regionMaxs[0] = region.getUpperX();
		cmd.sliceRegion.regionMaxs[1] = region.getUpperY();
		cmd.sliceRegion.regionMaxs[2] = region.getUpperZ();
		core::ScopedLock lock(_commandBufferMutex);
		_commandBuffer.push_back(cmd);
	}
};

using SceneRendererPtr = core::SharedPtr<ISceneRenderer>;

} // namespace voxedit
