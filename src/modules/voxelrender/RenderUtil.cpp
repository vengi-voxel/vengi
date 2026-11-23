/**
 * @file
 */

#include "RenderUtil.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNodeCamera.h"
#include "video/Camera.h"
#include "math/Quatf.h"
#include "math/QuatOps.h"
#include "math/Vec3f.h"
#include "math/Vec2i.h"
#include "math/Vec2f.h"
#include "math/Scalar.h"

namespace voxelrender {

scenegraph::SceneGraphNodeCamera toCameraNode(const video::Camera &camera) {
	scenegraph::SceneGraphNodeCamera cameraNode;
	const scenegraph::KeyFrameIndex keyFrameIdx = 0;
	scenegraph::SceneGraphTransform transform;
	transform.setWorldTranslation(camera.eye());
	transform.setWorldOrientation(camera.quaternion());
	cameraNode.setTransform(keyFrameIdx, transform);

	cameraNode.setAspectRatio(camera.aspect());
	cameraNode.setWidth(camera.size().x);
	cameraNode.setHeight(camera.size().y);
	cameraNode.setFarPlane(camera.farPlane());
	cameraNode.setNearPlane(camera.nearPlane());
	if (camera.isOrthographic()) {
		cameraNode.setOrthographic();
	} else {
		cameraNode.setPerspective();
	}
	cameraNode.setFieldOfView((int)camera.fieldOfView());
	cameraNode.setName("new camera");
	return cameraNode;
}

void configureCamera(video::Camera &camera, const voxel::Region &sceneRegion, SceneCameraMode mode, float farPlane,
					 const math::Vec3f &angles) {
	const math::Vec3f size(sceneRegion.getDimensionsInVoxels());

	// Determine visible dimensions based on camera mode
	float visibleWidth, visibleHeight;
	if (mode == SceneCameraMode::Top || mode == SceneCameraMode::Bottom) {
		// Looking down/up: see XZ plane
		visibleWidth = math::max(size.x, size.z);
		visibleHeight = math::min(size.x, size.z);
	} else if (mode == SceneCameraMode::Front || mode == SceneCameraMode::Back) {
		// Looking front/back: see XY plane
		visibleWidth = size.x;
		visibleHeight = size.y;
	} else if (mode == SceneCameraMode::Left || mode == SceneCameraMode::Right) {
		// Looking left/right: see ZY plane
		visibleWidth = size.z;
		visibleHeight = size.y;
	} else {
		// Free mode: diagonal view sees the XZ diagonal for width and Y height
		visibleWidth = math::length(math::Vec2f(size.x, size.z));
		visibleHeight = size.y;
	}

	const float fov = math::radians(camera.fieldOfView());
	const float aspect = (float)camera.size().y / (float)camera.size().x;
	const float distanceVertical = visibleHeight / (2.0f * math::tan(fov / 2.0f));
	const float distanceHorizontal = visibleWidth * aspect / (2.0f * math::tan(fov / 2.0f));
	const float distance = math::max(distanceVertical, distanceHorizontal) * 1.2f;

	const math::Vec3f &center = sceneRegion.calcCenterf();

	camera.resetZoom();
	camera.setRotationType(video::CameraRotationType::Target);
	camera.setAngles(angles[0], angles[1], angles[2]);
	camera.setTarget(center);
	camera.setTargetDistance(distance);
	const float neededFar = distance + math::length(size);
	camera.setFarPlane(core_max(farPlane, neededFar));
	if (mode != SceneCameraMode::Free) {
		camera.setOmega({0.0f, 0.0f, 0.0f});
	}
	if (mode == SceneCameraMode::Free) {
		if (camera.isIsometric()) {
			constexpr float isoYaw = math::radians(45.0f);
			camera.setAngles(video::Camera::IsometricPitch, isoYaw, 0.0f);
			return;
		}
		if (angles != math::Vec3f(0.0f)) {
			camera.update(0.0);
			return;
		}
		const float diagonalDistance = distance / math::sqrt(2.0f);
		camera.setWorldPosition(math::Vec3f(center.x - diagonalDistance, (float)sceneRegion.getUpperY(), center.z - diagonalDistance));
	} else if (mode == SceneCameraMode::Top) {
		camera.setWorldPosition(math::Vec3f(center.x, center.y + distance, center.z));
	} else if (mode == SceneCameraMode::Bottom) {
		camera.setWorldPosition(math::Vec3f(center.x, center.y - distance, center.z));
	} else if (mode == SceneCameraMode::Right) {
		camera.setWorldPosition(math::Vec3f(center.x + distance, center.y, center.z));
	} else if (mode == SceneCameraMode::Left) {
		camera.setWorldPosition(math::Vec3f(center.x - distance, center.y, center.z));
	} else if (mode == SceneCameraMode::Back) {
		camera.setWorldPosition(math::Vec3f(center.x, center.y, center.z + distance));
	} else if (mode == SceneCameraMode::Front) {
		camera.setWorldPosition(math::Vec3f(center.x, center.y, center.z - distance));
	}
	camera.lookAt(center);
}

math::Vec3f sunDirectionFromAngle(const math::Vec3f &angleDegrees) {
	const float pitch = math::radians(angleDegrees.x);
	const float yaw = math::radians(angleDegrees.y);
	return math::normalize(math::Vec3f(math::cos(pitch) * math::cos(yaw), math::sin(pitch), math::cos(pitch) * math::sin(yaw)));
}

static video::Camera toCamera(const math::Vec2i &size, const scenegraph::SceneGraphNodeCamera &cameraNode,
							  const math::Vec3f &worldPos, const math::Quatf &orientation) {
	video::Camera camera;
	// width, height and aspect of the cameraNode are not taken into account here
	camera.setSize(math::max(math::Vec2i(1, 1), size));
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

video::Camera toCamera(const math::Vec2i &size, const scenegraph::SceneGraph &sceneGraph,
					   const scenegraph::SceneGraphNodeCamera &cameraNode, scenegraph::FrameIndex frameIdx) {
	const scenegraph::FrameTransform &transform = sceneGraph.transformForFrame(cameraNode, frameIdx);
	return toCamera(size, cameraNode, transform.worldTranslation(), math::Quatf(transform.worldMatrix()));
}

video::Camera toCamera(const math::Vec2i &size, const scenegraph::SceneGraphNodeCamera &cameraNode) {
	scenegraph::KeyFrameIndex keyFrameIdx = 0;
	const scenegraph::SceneGraphTransform &transform = cameraNode.transform(keyFrameIdx);
	return toCamera(size, cameraNode, transform.worldTranslation(), transform.worldOrientation());
}

} // namespace voxelrender
