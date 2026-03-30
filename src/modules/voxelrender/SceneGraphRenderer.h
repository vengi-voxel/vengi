/**
 * @file
 */

#pragma once

#include "RawVolumeRenderer.h"
#include "core/SharedPtr.h"
#include "core/collection/Buffer.h"
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
	bool sliceViewActiveForNode(int nodeId) const;
	bool sliceViewActive() const;
	void updateNodeState(const voxel::MeshStatePtr &meshState, const RenderContext &renderContext,
						 const scenegraph::SceneGraphNode &activeNode, const scenegraph::SceneGraphNode &node,
						 int idx) const;
	void applyTransform(const voxel::MeshStatePtr &meshState, const RenderContext &renderContext,
						const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node, int idx);
	void prepareModelNodes(const voxel::MeshStatePtr &meshState, const RenderContext &renderContext);
	void prepareReferenceNodes(const voxel::MeshStatePtr &meshState, const RenderContext &renderContext) const;
	void prepareCameraNodes(const RenderContext &renderContext);

	void prepare(const voxel::MeshStatePtr &meshState, const RenderContext &renderContext);

	core::SharedPtr<voxel::RawVolume> _sliceVolume;
	voxel::Region _sliceRegion = voxel::Region::InvalidRegion;
	bool _sliceVolumeDirty = false;
	int _sliceVolumeNodeId = -1;

	/** @brief Sparse table indexed by nodeId, value is the compact volume index (-1 = unmapped) */
	core::Buffer<int> _nodeIdToVolumeIdx;
	/** @brief Free-list of recycled compact volume indices */
	core::DynamicStack<int> _freeVolumeIndices;
	/** @brief High-water mark for next new volume index allocation */
	int _nextVolumeIdx = 0;

	int allocateVolumeIdx(int nodeId);
	void freeVolumeIdx(int nodeId);

public:
	SceneGraphRenderer();
	void construct();
	bool init(bool normals);
	void update(const voxel::MeshStatePtr &meshState);
	void shutdown();

	void setAmbientColor(const glm::vec3 &color);
	void setDiffuseColor(const glm::vec3 &color);
	void setSunAngle(const glm::vec3 &angle);

	void nodeRemove(const voxel::MeshStatePtr &meshState, int nodeId);
	/**
	 * @brief Checks whether the given model node is visible
	 * @param[in] nodeId The node id
	 * @param[in] hideEmpty If @c true, the function will return @c false if the volume is empty
	 * @return @c true if the node is visible, @c false otherwise
	 */
	bool isVisible(const voxel::MeshStatePtr &meshState, int nodeId, bool hideEmpty = true) const;

	void scheduleRegionExtraction(const voxel::MeshStatePtr &meshState, int nodeId, const voxel::Region &region);
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

	inline int getVolumeIdx(int nodeId) const {
		if (nodeId < 0 || nodeId >= (int)_nodeIdToVolumeIdx.size()) {
			return -1;
		}
		return _nodeIdToVolumeIdx[nodeId];
	}

	inline int getVolumeIdx(const scenegraph::SceneGraphNode &node) const {
		return getVolumeIdx(node.id());
	}

	int getOrAssignVolumeIdx(int nodeId);
};

} // namespace voxelrender
