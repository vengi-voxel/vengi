/**
 * @file
 */

#include "Controller.h"
#include "frontend/Movement.h"

namespace voxedit {

void Controller::resetCamera(const voxel::Region& region) {
	_camera.setAngles(0.0f, 0.0f, 0.0f);
	const glm::ivec3& center = region.getCentre();
	_camera.setTarget(center);
	const glm::vec3 dim(region.getDimensionsInVoxels());
	const float distance = glm::length(dim);
	_camera.setTargetDistance(distance * 2.0f);
	if (_camMode == Controller::SceneCameraMode::Free) {
		const int height = region.getHeightInCells();
		_camera.setPosition(glm::vec3(-distance, height + distance, -distance));
	} else if (_camMode == Controller::SceneCameraMode::Top) {
		const int height = region.getHeightInCells();
		_camera.setPosition(glm::vec3(center.x, height + center.y, center.z));
	} else if (_camMode == Controller::SceneCameraMode::Left) {
		_camera.setPosition(glm::vec3(-center.x, center.y, center.z));
	} else if (_camMode == Controller::SceneCameraMode::Front) {
		const int depth = region.getDepthInCells();
		_camera.setPosition(glm::vec3(center.x, center.y, -depth - center.z));
	}
	_camera.lookAt(center);
	_camera.setFarPlane(5000.0f);
}

void Controller::update(long deltaFrame) {
	_camera.update(deltaFrame);
}

void Controller::init(Controller::SceneCameraMode mode) {
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

void Controller::onResize(const glm::ivec2& size) {
	_camera.init(glm::ivec2(0), size);
}

void Controller::zoom(float level) {
	const float value = _cameraSpeed * level;
	const float targetDistance = glm::clamp(_camera.targetDistance() + value, 0.0f, 1000.0f);
	if (targetDistance > 1.0f) {
		const glm::vec3& moveDelta = glm::backward * value;
		_camera.move(moveDelta);
		_camera.setTargetDistance(targetDistance);
	}
}

bool Controller::move(bool rotate, int x, int y) {
	if (rotate) {
		const float yaw = x - _mouseX;
		const float pitch = y - _mouseY;
		const float s = _rotationSpeed->floatVal();
		if (_camMode == Controller::SceneCameraMode::Free) {
			_camera.turn(yaw * s);
			_camera.pitch(pitch * s);
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
