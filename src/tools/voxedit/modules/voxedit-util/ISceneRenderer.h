/**
 * @file
 */

#pragma once

#include "core/IComponent.h"
#include "core/SharedPtr.h"
#include "core/Trace.h"
#include "core/UUID.h"
#include "core/collection/DynamicArray.h"
#include "core/concurrent/Lock.h"
#include "voxel/Region.h"

namespace scenegraph {
class SceneGraph;
class SceneGraphNode;
}

namespace video {
class Camera;
}

namespace voxel {
class RawVolume;
}

namespace voxelrender {
struct RenderContext;
}

namespace voxedit {

struct AddNodePreview;

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
				uint64_t uuid[2];
				int32_t regionMins[3];
				int32_t regionMaxs[3];
			} nodeRegion;

			struct {
				int32_t regionMins[3];
				int32_t regionMaxs[3];
			} sliceRegion;

			struct {
				uint64_t uuid[2];
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

	virtual bool isVisible(const core::UUID &uuid, bool hideEmpty = true) const {
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
	virtual const voxel::RawVolume *volumeForNode(const scenegraph::SceneGraphNode &node);

	virtual const voxel::Region &sliceRegion() const {
		return voxel::Region::InvalidRegion;
	}

	virtual bool isSliceModeActive() const {
		return sliceRegion().isValid();
	}

	virtual RendererStats rendererStats() const {
		return {};
	}

	virtual void updateNodeRegion(const core::UUID &uuid, const voxel::Region &region) {
		CommandEvent cmd;
		cmd.type = CommandType::NodeRegion;
		cmd.nodeRegion.uuid[0] = uuid.data0();
		cmd.nodeRegion.uuid[1] = uuid.data1();
		cmd.nodeRegion.regionMins[0] = region.getLowerX();
		cmd.nodeRegion.regionMins[1] = region.getLowerY();
		cmd.nodeRegion.regionMins[2] = region.getLowerZ();
		cmd.nodeRegion.regionMaxs[0] = region.getUpperX();
		cmd.nodeRegion.regionMaxs[1] = region.getUpperY();
		cmd.nodeRegion.regionMaxs[2] = region.getUpperZ();
		core::ScopedLock lock(_commandBufferMutex);
		_commandBuffer.push_back(cmd);
	}

	virtual void removeNode(const core::UUID &uuid) {
		CommandEvent cmd;
		cmd.type = CommandType::RemoveNode;
		cmd.node.uuid[0] = uuid.data0();
		cmd.node.uuid[1] = uuid.data1();
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

	virtual void unhideNode(const core::UUID &uuid) {
		CommandEvent cmd;
		cmd.type = CommandType::UnhideNode;
		cmd.node.uuid[0] = uuid.data0();
		cmd.node.uuid[1] = uuid.data1();
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

	virtual void setAddNodePreview(const AddNodePreview &preview) {
		(void)preview;
	}
};

using SceneRendererPtr = core::SharedPtr<ISceneRenderer>;

} // namespace voxedit
