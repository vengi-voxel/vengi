/**
 * @file
 */

#include "Gizmo.h"
#include "core/GLM.h"
#include "video/Ray.h"
#include "video/Camera.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/intersect.hpp>
#include <glm/gtx/norm.hpp>

namespace render {

static constexpr float GizmoSize = 20.0f;
static constexpr glm::vec3 GizmoSizeVec(GizmoSize);

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

GizmoMode Gizmo::mode() const {
	return _mode;
}

void Gizmo::setPosition(const glm::vec3& pos) {
	_pos = pos;
	_axis.setPosition(pos);
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

	const bool intersectX = glm::intersectLines(p1, p2, glm::zero<glm::vec3>(), _pos + glm::vec3(GizmoSize,  0.0f,  0.0f), paX, pbX);
	const bool intersectY = glm::intersectLines(p1, p2, glm::zero<glm::vec3>(), _pos + glm::vec3( 0.0f, GizmoSize,  0.0f), paY, pbY);
	const bool intersectZ = glm::intersectLines(p1, p2, glm::zero<glm::vec3>(), _pos + glm::vec3( 0.0f,  0.0f, GizmoSize), paZ, pbZ);
	const float distanceX = intersectX ? glm::distance2(paX, pbX) : FLT_MAX;
	const float distanceY = intersectY ? glm::distance2(paY, pbY) : FLT_MAX;
	const float distanceZ = intersectZ ? glm::distance2(paZ, pbZ) : FLT_MAX;

	if (glm::any(glm::lessThan(paX, glm::zero<glm::vec3>())) || glm::any(glm::greaterThan(paX, GizmoSizeVec))
	 || glm::any(glm::lessThan(paY, glm::zero<glm::vec3>())) || glm::any(glm::greaterThan(paY, GizmoSizeVec))
	 || glm::any(glm::lessThan(paZ, glm::zero<glm::vec3>())) || glm::any(glm::greaterThan(paZ, GizmoSizeVec))) {
		_mode = GizmoMode::None;
		_axis.setSize(GizmoSize, GizmoSize, GizmoSize);
		return;
	}

	const float distanceToLine = 0.2f;
	if (distanceX < distanceY && distanceX < distanceZ && distanceX < distanceToLine) {
		_mode = GizmoMode::TranslateX;
		_axis.setSize(2.0f * GizmoSize, GizmoSize, GizmoSize);
	} else if (distanceY < distanceX && distanceY < distanceZ && distanceY < distanceToLine) {
		_mode = GizmoMode::TranslateY;
		_axis.setSize(GizmoSize, 2.0f * GizmoSize, GizmoSize);
	} else if (distanceZ < distanceX && distanceZ < distanceY && distanceZ < distanceToLine) {
		_mode = GizmoMode::TranslateZ;
		_axis.setSize(GizmoSize, GizmoSize, 2.0f * GizmoSize);
	} else {
		_mode = GizmoMode::None;
		_axis.setSize(GizmoSize, GizmoSize, GizmoSize);
	}
}

bool Gizmo::handleDown(int32_t key, double pressedMillis) {
	const bool initialDown = core::ActionButton::handleDown(key, pressedMillis);
	if (initialDown) {
		_buttonMode = _mode;
		_buttonLastAction = 0;
	}
	return initialDown;
}

bool Gizmo::handleUp(int32_t key, double releasedMillis) {
	const bool ret = core::ActionButton::handleUp(key, releasedMillis);
	_buttonMode = GizmoMode::None;
	_buttonLastPosition = glm::ivec3(0);
	return ret;
}

bool Gizmo::execute(double nowSeconds, const std::function<glm::ivec3(const glm::ivec3, GizmoMode)>& callback) {
	if (!pressed()) {
		return false;
	}
	if (_buttonMode == GizmoMode::None) {
		return false;
	}
	if (nowSeconds - _buttonLastAction < 0.5) {
		return false;
	}
	_buttonLastAction = nowSeconds;
	_buttonLastPosition = callback(_buttonLastPosition, _buttonMode);
	return true;
}

}
