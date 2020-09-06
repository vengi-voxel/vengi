/**
 * @file
 */

#include "ViewportController.h"

namespace voxedit {

void ViewportController::resetCamera(const voxel::Region& region) {
	_camera.setAngles(0.0f, 0.0f, 0.0f);
	_camera.setFarPlane(5000.0f);
	const glm::ivec3& center = region.getCenter();
	if (_renderMode == RenderMode::Animation) {
		_camera.setTarget(glm::zero<glm::vec3>());
		_camera.setPosition(glm::vec3(10.0f, 5.0f, 10.0f));
		_camera.setTargetDistance(10.0f);
		return;
	}
	_camera.setTarget(center);
	const glm::vec3 dim(region.getDimensionsInVoxels());
	const float distance = glm::length(dim);
	_camera.setTargetDistance(distance * 2.0f);
	if (_camMode == SceneCameraMode::Free) {
		const int height = region.getHeightInCells();
		_camera.setPosition(glm::vec3(-distance, height + distance, -distance));
	} else if (_camMode == SceneCameraMode::Top) {
		const int height = region.getHeightInCells();
		_camera.setPosition(glm::vec3(center.x, height + center.y, center.z));
	} else if (_camMode == SceneCameraMode::Left) {
		_camera.setPosition(glm::vec3(-center.x, center.y, center.z));
	} else if (_camMode == SceneCameraMode::Front) {
		const int depth = region.getDepthInCells();
		_camera.setPosition(glm::vec3(center.x, center.y, -depth - center.z));
	}
}

void ViewportController::update(double deltaFrameSeconds) {
	_camera.update(deltaFrameSeconds);
}

void ViewportController::init(ViewportController::SceneCameraMode mode) {
	_camera.setRotationType(video::CameraRotationType::Target);
	_camMode = mode;
	switch (mode) {
	case SceneCameraMode::Top:
	case SceneCameraMode::Front:
	case SceneCameraMode::Left:
		// TODO: make ortho
		_camera.setMode(video::CameraMode::Perspective);
		break;
	case SceneCameraMode::Free:
		_camera.setMode(video::CameraMode::Perspective);
		break;
	}
	_rotationSpeed = core::Var::getSafe(cfg::ClientMouseRotationSpeed);
}

void ViewportController::onResize(const glm::ivec2& frameBufferSize, const glm::ivec2& windowSize) {
	_camera.init(glm::ivec2(0), frameBufferSize, windowSize);
}

bool ViewportController::move(bool rotate, int x, int y) {
	if (rotate) {
		const float yaw = (float)(x - _mouseX);
		const float pitch = (float)(y - _mouseY);
		const float s = _rotationSpeed->floatVal();
		if (_camMode == SceneCameraMode::Free) {
			_camera.turn(yaw * s);
			_camera.setPitch(pitch * s);
		}
		_mouseX = x;
		_mouseY = y;
		return false;
	}
	_mouseX = x;
	_mouseY = y;
	return true;
}

}
