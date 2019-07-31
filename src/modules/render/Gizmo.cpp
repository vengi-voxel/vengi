/**
 * @file
 */

#include "Gizmo.h"
#include "core/GLM.h"
#include <glm/gtx/intersect.hpp>
#include <glm/gtx/norm.hpp>

namespace render {

bool Gizmo::init() {
	if (!_axis.init()) {
		return false;
	}

	return true;
}

void Gizmo::shutdown() {
	_axis.shutdown();
}

void Gizmo::render(const video::Camera& camera) {
	_axis.render(camera);
}

Gizmo::Mode Gizmo::mode() const {
	return _mode;
}

void Gizmo::setPosition(const glm::vec3& pos) {
	_pos = pos;
}

void Gizmo::update(const video::Camera& camera, const glm::ivec2& pixelPos) {
	// don't update the cached state if we have the button pressed already.
	if (pressed()) {
		return;
	}
	static glm::ivec2 lastPixelPos = pixelPos;
	if (lastPixelPos == pixelPos) {
		return;
	}
	lastPixelPos = pixelPos;
	const video::Ray& ray = camera.mouseRay(pixelPos);

	const glm::vec3 p1 = ray.origin;
	const glm::vec3 p2 = p1 + ray.direction * camera.farPlane();

	glm::vec3 paX;
	glm::vec3 pbX;
	glm::vec3 paY;
	glm::vec3 pbY;
	glm::vec3 paZ;
	glm::vec3 pbZ;

	const bool intersectX = glm::intersectLines(p1, p2, glm::zero<glm::vec3>(), glm::vec3(20.0f,  0.0f,  0.0f), paX, pbX);
	const bool intersectY = glm::intersectLines(p1, p2, glm::zero<glm::vec3>(), glm::vec3( 0.0f, 20.0f,  0.0f), paY, pbY);
	const bool intersectZ = glm::intersectLines(p1, p2, glm::zero<glm::vec3>(), glm::vec3( 0.0f,  0.0f, 20.0f), paZ, pbZ);
	const float distanceX = intersectX ? glm::distance2(paX, pbX) : FLT_MAX;
	const float distanceY = intersectY ? glm::distance2(paY, pbY) : FLT_MAX;
	const float distanceZ = intersectZ ? glm::distance2(paZ, pbZ) : FLT_MAX;

	const float distanceToLine = 0.2f;
	if (distanceX < distanceY && distanceX < distanceZ && distanceX < distanceToLine) {
		_mode = Mode::TranslateX;
		_axis.setSize(40.0f, 20.0f, 20.0f);
	} else if (distanceY < distanceX && distanceY < distanceZ && distanceY < distanceToLine) {
		_mode = Mode::TranslateY;
		_axis.setSize(20.0f, 40.0f, 20.0f);
	} else if (distanceZ < distanceX && distanceZ < distanceY && distanceZ < distanceToLine) {
		_mode = Mode::TranslateZ;
		_axis.setSize(20.0f, 20.0f, 40.0f);
	} else {
		_mode = Mode::None;
		_axis.setSize(20.0f, 20.0f, 20.0f);
	}
}

bool Gizmo::handleDown(int32_t key, uint64_t pressedMillis) {
	const bool ret = core::ActionButton::handleDown(key, pressedMillis);
	_buttonMode = _mode;
	return ret;
}

bool Gizmo::handleUp(int32_t key, uint64_t releasedMillis) {
	const bool ret = core::ActionButton::handleUp(key, pressedMillis);
	_buttonMode = render::Gizmo::Mode::None;
	return ret;
}

bool Gizmo::execute(uint64_t time, std::function<glm::ivec3(const glm::ivec3, render::Gizmo::Mode)> function) {
	if (!pressed()) {
		return false;
	}
	if (_buttonMode == render::Gizmo::Mode::None) {
		return false;
	}
	if (time - _buttonLastAction < 125ul) {
		return false;
	}
	_buttonLastAction = time;
	_buttonLastPosition = function(_buttonLastPosition, _buttonMode);
	return true;
}

}
