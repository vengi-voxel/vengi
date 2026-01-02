/**
 * @file
 */

#include "RenderUtil.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNodeCamera.h"
#include "video/Camera.h"

namespace voxelrender {

scenegraph::SceneGraphNodeCamera toCameraNode(const video::Camera &camera) {
	scenegraph::SceneGraphNodeCamera cameraNode;
	const scenegraph::KeyFrameIndex keyFrameIdx = 0;
	scenegraph::SceneGraphTransform transform;
	transform.setWorldTranslation(camera.eye());
	transform.setWorldOrientation(camera.orientation());
	cameraNode.setTransform(keyFrameIdx, transform);

	cameraNode.setAspectRatio(camera.aspect());
	cameraNode.setWidth(camera.size().x);
	cameraNode.setHeight(camera.size().y);
	cameraNode.setFarPlane(camera.farPlane());
	cameraNode.setNearPlane(camera.nearPlane());
	if (camera.mode() == video::CameraMode::Orthogonal) {
		cameraNode.setOrthographic();
	} else {
		cameraNode.setPerspective();
	}
	cameraNode.setFieldOfView((int)camera.fieldOfView());
	cameraNode.setName("new camera");
	return cameraNode;
}

void configureCamera(video::Camera &camera, const voxel::Region &sceneRegion, SceneCameraMode mode, float farPlane,
					 const glm::vec3 &angles) {
	const glm::vec3 size(sceneRegion.getDimensionsInVoxels());
	const float maxDim = (float)glm::max(size.x, glm::max(size.y, size.z));
	const float distance = maxDim * 2.0f;
	const glm::vec3 &center = sceneRegion.calcCenterf();

	camera.resetZoom();
	camera.setRotationType(video::CameraRotationType::Target);
	camera.setAngles(angles[0], angles[1], angles[2]);
	camera.setTarget(center);
	camera.setTargetDistance(distance);
	camera.setFarPlane(farPlane);
	if (mode != SceneCameraMode::Free) {
		camera.setOmega({0.0f, 0.0f, 0.0f});
	}
	if (mode == SceneCameraMode::Free) {
		camera.setWorldPosition(glm::vec3(center.x - distance, (float)sceneRegion.getUpperY(), center.z - distance));
	} else if (mode == SceneCameraMode::Top) {
		camera.setWorldPosition(glm::vec3(center.x, center.y + size.y, center.z));
	} else if (mode == SceneCameraMode::Bottom) {
		camera.setWorldPosition(glm::vec3(center.x, center.y - size.y, center.z));
	} else if (mode == SceneCameraMode::Right) {
		camera.setWorldPosition(glm::vec3(center.x + size.x, center.y, center.z));
	} else if (mode == SceneCameraMode::Left) {
		camera.setWorldPosition(glm::vec3(center.x - size.x, center.y, center.z));
	} else if (mode == SceneCameraMode::Back) {
		camera.setWorldPosition(glm::vec3(center.x, center.y, center.z + size.z));
	} else if (mode == SceneCameraMode::Front) {
		camera.setWorldPosition(glm::vec3(center.x, center.y, center.z - size.z));
	}
	camera.lookAt(center);
}

static video::Camera toCamera(const glm::ivec2 &size, const scenegraph::SceneGraphNodeCamera &cameraNode,
							  const glm::vec3 &worldPos, const glm::quat &orientation) {
	video::Camera camera;
	// width, height and aspect of the cameraNode are not taken into account here
	camera.setSize(glm::max(glm::ivec2(1, 1), size));
	if (cameraNode.isOrthographic()) {
		camera.setMode(video::CameraMode::Orthogonal);
	} else {
		camera.setMode(video::CameraMode::Perspective);
	}
	float fplane = cameraNode.farPlane();
	float nplane = cameraNode.nearPlane();
	if (fplane > nplane) {
		camera.setFarPlane(fplane);
		camera.setNearPlane(nplane);
	}
	camera.setWorldPosition(worldPos);
	camera.setOrientation(orientation);
	const int fovDegree = cameraNode.fieldOfView();
	if (fovDegree > 0) {
		camera.setFieldOfView((float)fovDegree);
	}
	camera.update(0.0);
	return camera;
}

video::Camera toCamera(const glm::ivec2 &size, const scenegraph::SceneGraph &sceneGraph,
					   const scenegraph::SceneGraphNodeCamera &cameraNode, scenegraph::FrameIndex frameIdx) {
	const scenegraph::FrameTransform &transform = sceneGraph.transformForFrame(cameraNode, frameIdx);
	return toCamera(size, cameraNode, transform.worldTranslation(), glm::quat(transform.worldMatrix()));
}

video::Camera toCamera(const glm::ivec2 &size, const scenegraph::SceneGraphNodeCamera &cameraNode) {
	scenegraph::KeyFrameIndex keyFrameIdx = 0;
	const scenegraph::SceneGraphTransform &transform = cameraNode.transform(keyFrameIdx);
	return toCamera(size, cameraNode, transform.worldTranslation(), transform.worldOrientation());
}

} // namespace voxelrender
