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
	enum class CommandType : uint8_t { NodeRegion, SliceRegion, RemoveNode, UnhideNode, Clear, MarkDirty };

	struct CommandEvent {
		CommandType type;
		union {
			struct {
				int nodeId;
				int32_t regionMins[3];
				int32_t regionMaxs[3];
			} nodeRegion;

			struct {
				int32_t regionMins[3];
				int32_t regionMaxs[3];
			} sliceRegion;

			struct {
				int nodeId;
			} node;
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

	virtual void updateNodeRegion(int nodeId, const voxel::Region &region) {
		CommandEvent cmd;
		cmd.type = CommandType::NodeRegion;
		cmd.nodeRegion.nodeId = nodeId;
		cmd.nodeRegion.regionMins[0] = region.getLowerX();
		cmd.nodeRegion.regionMins[1] = region.getLowerY();
		cmd.nodeRegion.regionMins[2] = region.getLowerZ();
		cmd.nodeRegion.regionMaxs[0] = region.getUpperX();
		cmd.nodeRegion.regionMaxs[1] = region.getUpperY();
		cmd.nodeRegion.regionMaxs[2] = region.getUpperZ();
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
