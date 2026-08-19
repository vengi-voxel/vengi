/**
 * @file
 */

#pragma once

#include "RawVolumeRenderer.h"
#include "core/SharedPtr.h"
#include "core/UUID.h"
#include "core/collection/Buffer.h"
#include "core/collection/DynamicMap.h"
#include "core/collection/DynamicStack.h"
#include "render/CameraRenderer.h"
#include "scenegraph/SceneGraphNode.h"
#include "video/Camera.h"
#include "voxel/RawVolume.h"

/**
 * Basic voxel rendering
 */
namespace voxelrender {

/**
 * @brief Rendering of a @c voxel::MeshState
 */
class SceneGraphRenderer : public core::NonCopyable {
protected:
	RawVolumeRenderer _volumeRenderer;
	render::CameraRenderer _cameraRenderer;
	core::Buffer<render::CameraRenderer::Node> _cameras;
	void prepareMeshStateTransform(const voxel::MeshStatePtr &meshState, const scenegraph::SceneGraph &sceneGraph,
								   const scenegraph::FrameIndex &frame, const scenegraph::SceneGraphNode &node, int idx) const;
	void handleSliceView(const voxel::MeshStatePtr &meshState, scenegraph::SceneGraphNode &node);
	bool sliceViewActiveForNode(const core::UUID &uuid) const;
	bool sliceViewActive() const;
	void updateNodeState(const voxel::MeshStatePtr &meshState, const RenderContext &renderContext,
						 const scenegraph::SceneGraphNode &activeNode, const scenegraph::SceneGraphNode &node,
						 int idx) const;
	void applyTransform(const voxel::MeshStatePtr &meshState, const RenderContext &renderContext,
						const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node, int idx);
	void prepareModelNodes(const voxel::MeshStatePtr &meshState, const RenderContext &renderContext);
	void prepareReferenceNodes(const voxel::MeshStatePtr &meshState, const RenderContext &renderContext);
	void prepareCameraNodes(const RenderContext &renderContext);

	struct VisibleNode {
		int nodeId;
		int idx;
	};
	core::Buffer<VisibleNode> _visibleNodesScratch;

	void prepare(const voxel::MeshStatePtr &meshState, const RenderContext &renderContext);

	core::SharedPtr<voxel::RawVolume> _sliceVolume;
	voxel::Region _sliceRegion = voxel::Region::InvalidRegion;
	bool _sliceVolumeDirty = false;
	core::UUID _sliceVolumeNodeUUID;

	/** @brief Maps stable node UUIDs to compact volume indices. */
	core::DynamicMap<core::UUID, int, 251, core::UUIDHash> _uuidToVolumeIdx;
	/** @brief Free-list of recycled compact volume indices */
	core::DynamicStack<int> _freeVolumeIndices;
	/** @brief High-water mark for next new volume index allocation */
	int _nextVolumeIdx = 0;

	int allocateVolumeIdx(const core::UUID &uuid);
	void freeVolumeIdx(const core::UUID &uuid);

public:
	SceneGraphRenderer(const core::TimeProviderPtr &timeProvider);
	void construct();
	bool init(bool normals);
	void update(const voxel::MeshStatePtr &meshState);
	void shutdown();

	void setAmbientColor(const glm::vec3 &color);
	void setDiffuseColor(const glm::vec3 &color);
	void setSunAngle(const glm::vec3 &angle);

	void nodeRemove(const voxel::MeshStatePtr &meshState, const core::UUID &uuid);
	/**
	 * @brief Checks whether the given model node is visible
	 * @param[in] nodeId The node id
	 * @param[in] hideEmpty If @c true, the function will return @c false if the volume is empty
	 * @return @c true if the node is visible, @c false otherwise
	 */
	bool isVisible(const voxel::MeshStatePtr &meshState, const core::UUID &uuid, bool hideEmpty = true) const;

	void scheduleRegionExtraction(const voxel::MeshStatePtr &meshState, const core::UUID &uuid,
								  const voxel::Region &region);
	/**
	 * @param waitPending Wait for pending extractions and update the buffers before doing the rendering. If this is
	 * false, you have to call @c update() manually!
	 */
	void render(const voxel::MeshStatePtr &meshState, RenderContext &renderContext, const video::Camera &camera,
				bool shadow = true, bool waitPending = false);
	void clear(const voxel::MeshStatePtr &meshState);

	const voxel::Region &sliceRegion() const;
	void setSliceRegion(const voxel::Region &region);
	bool isSliceModeActive() const;

	int culledVolumeCount() const {
		return _volumeRenderer.culledVolumeCount();
	}

	int freeVolumeIndexCount() const {
		return (int)_freeVolumeIndices.size();
	}

	int nextVolumeIdx() const {
		return _nextVolumeIdx;
	}

	int mappedNodeCount() const {
		return _nextVolumeIdx - (int)_freeVolumeIndices.size();
	}

	inline int getVolumeIdx(const core::UUID &uuid) const {
		int idx = -1;
		_uuidToVolumeIdx.get(uuid, idx);
		return idx;
	}

	inline int getVolumeIdx(const scenegraph::SceneGraphNode &node) const {
		return getVolumeIdx(node.uuid());
	}

	int getOrAssignVolumeIdx(const core::UUID &uuid);
	inline int getOrAssignVolumeIdx(const scenegraph::SceneGraphNode &node) {
		return getOrAssignVolumeIdx(node.uuid());
	}
};

} // namespace voxelrender
