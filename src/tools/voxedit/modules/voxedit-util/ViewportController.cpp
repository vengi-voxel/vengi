/**
 * @file
 */

#include "ViewportController.h"

namespace voxedit {

void ViewportController::resetCamera(const glm::ivec3 &pos, const voxel::Region &region) {
	_camera.setAngles(0.0f, 0.0f, 0.0f);
	_camera.setFarPlane(5000.0f);
	_camera.setTarget(pos);
	const float distance = 100.0f;
	glm::ivec3 center = pos;
	if (region.isValid()) {
		center = region.getCenter();
	}
	_camera.setTargetDistance(distance);
	if (_renderMode == RenderMode::Animation) {
		_camera.setTarget(pos);
		const int height = region.getHeightInCells();
		_camera.setWorldPosition(glm::vec3(-distance, (float)height + distance, -distance));
	} else if (_camMode == SceneCameraMode::Free) {
		const int height = region.getHeightInCells();
		_camera.setWorldPosition(glm::vec3(-distance, (float)height + distance, -distance));
	} else if (_camMode == SceneCameraMode::Top) {
		const int height = region.getHeightInCells();
		_camera.setWorldPosition(glm::vec3(center.x, height + center.y, center.z));
	} else if (_camMode == SceneCameraMode::Left) {
		_camera.setWorldPosition(glm::vec3(-center.x, center.y, center.z));
	} else if (_camMode == SceneCameraMode::Front) {
		const int depth = region.getDepthInCells();
		_camera.setWorldPosition(glm::vec3(center.x, center.y, -depth - center.z));
	}
}

void ViewportController::update(double deltaFrameSeconds) {
	_camera.update(deltaFrameSeconds);
}

void ViewportController::setMode(ViewportController::SceneCameraMode mode) {
	_camMode = mode;
	if (mode == ViewportController::SceneCameraMode::Free) {
		_camera.setMode(video::CameraMode::Perspective);
	} else {
		_camera.setMode(video::CameraMode::Orthogonal);
	}
}

bool ViewportController::init() {
	_rotationSpeed = core::Var::getSafe(cfg::ClientMouseRotationSpeed);
	_camera.setAngles(0.0f, 0.0f, 0.0f);
	_camera.setFarPlane(5000.0f);
	_camera.setRotationType(video::CameraRotationType::Target);
	return true;
}

void ViewportController::onResize(const glm::ivec2&, const glm::ivec2& windowSize) {
	_camera.setSize(windowSize);
}

void ViewportController::move(bool pan, bool rotate, int x, int y) {
	if (rotate) {
		const float yaw = (float)(x - _mouseX);
		const float pitch = (float)(y - _mouseY);
		const float s = _rotationSpeed->floatVal();
		if (_camMode == SceneCameraMode::Free) {
			_camera.turn(yaw * s);
			_camera.setPitch(pitch * s);
		}
	} else if (pan) {
		_camera.pan(x - _mouseX, y - _mouseY);
	}
	_mouseX = x;
	_mouseY = y;
}

}
