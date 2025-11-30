/**
 * @file
 */

#pragma once

#include "RawVolumeRenderer.h"
#include "app/I18N.h"
#include "core/SharedPtr.h"
#include "core/collection/Buffer.h"
#include "render/CameraFrustum.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphNodeCamera.h"
#include "video/Camera.h"
#include "voxel/RawVolume.h"

/**
 * Basic voxel rendering
 */
namespace voxelrender {

/**
 * @see voxelformat::toCameraNode()
 */
video::Camera toCamera(const glm::ivec2 &size, const scenegraph::SceneGraphNodeCamera &cameraNode);
video::Camera toCamera(const glm::ivec2 &size, const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNodeCamera &cameraNode, scenegraph::FrameIndex frameIdx);
scenegraph::SceneGraphNodeCamera toCameraNode(const video::Camera &camera);
enum class SceneCameraMode : uint8_t { Free, Top, Bottom, Left, Right, Front, Back, Max };
// I18N: These are not translated, because they are also the values of configuration variables
static constexpr const char *SceneCameraModeStr[] = {N_("Free"), N_("Top"), N_("Bottom"), N_("Left"), N_("Right"), N_("Front"), N_("Back")};
static_assert(lengthof(SceneCameraModeStr) == (int)voxelrender::SceneCameraMode::Max, "Array size doesn't match enum values");

/**
 * @brief Tries to place the camera in a way that most of the scene region is visible in the viewport of the camera.
 * @param[in,out] camera The camera to configure
 * @param[in] angles The angles to set the camera to (pitch, yaw, roll in radians)
 */
void configureCamera(video::Camera &camera, const voxel::Region &sceneRegion, SceneCameraMode mode, float farPlane,
					 const glm::vec3 &angles = {0.0f, 0.0f, 0.0f});

/**
 * @brief Rendering of a @c voxel::MeshState
 */
class SceneGraphRenderer : public core::NonCopyable {
protected:
	RawVolumeRenderer _volumeRenderer;
	render::CameraFrustum _cameraRenderer;
	core::Buffer<video::Camera> _cameras;
	void prepareMeshStateTransform(const voxel::MeshStatePtr &meshState, const scenegraph::SceneGraph &sceneGraph,
								   const scenegraph::FrameIndex &frame, const scenegraph::SceneGraphNode &node, int idx,
								   const voxel::Region &region);
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
	void render(const voxel::MeshStatePtr &meshState, RenderContext &renderContext, const video::Camera &camera, bool shadow = true,
				bool waitPending = false);
	void clear(const voxel::MeshStatePtr &meshState);

	const voxel::Region &sliceRegion() const;
	void setSliceRegion(const voxel::Region &region);
	bool isSliceModeActive() const;

	static inline int getVolumeIdx(int nodeId) {
		// TODO: using the node id here is not good as they are increasing when you modify the scene graph
		return nodeId;
	}

	static inline int getVolumeIdx(const scenegraph::SceneGraphNode &node) {
		return getVolumeIdx(node.id());
	}

	static inline int getNodeId(int volumeIdx) {
		// TODO: using the node id here is not good as they are increasing when you modify the scene graph
		return volumeIdx;
	}
};

} // namespace voxelrender
