/**
 * @file
 */

#pragma once

#include "RawVolumeRenderer.h"
#include "core/SharedPtr.h"
#include "core/collection/Buffer.h"
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

	/** @brief remove those volumes that are no longer part of the scene graph */
	void resetVolumes(const voxel::MeshStatePtr &meshState, const scenegraph::SceneGraph &sceneGraph);
	void prepare(const voxel::MeshStatePtr &meshState, const RenderContext &renderContext);

	core::SharedPtr<voxel::RawVolume> _sliceVolume;
	voxel::Region _sliceRegion = voxel::Region::InvalidRegion;
	bool _sliceVolumeDirty = false;
	int _sliceVolumeNodeId = -1;

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

	static inline int getVolumeIdx(int nodeId) {
		// TODO: using the node id here is not good as they are increasing when you modify the scene graph - and there
		// is a max limit that could get reached here even though you only have one node active
		return nodeId;
	}

	static inline int getNodeId(int volumeIdx) {
		// TODO: using the node id here is not good as they are increasing when you modify the scene graph - and there
		// is a max limit that could get reached here even though you only have one node active
		return volumeIdx;
	}

	static inline int getVolumeIdx(const scenegraph::SceneGraphNode &node) {
		return getVolumeIdx(node.id());
	}
};

} // namespace voxelrender
