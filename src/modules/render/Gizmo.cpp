/**
 * @file
 */

#include "Gizmo.h"
#include "core/Enum.h"
#include "core/GLM.h"
#include "core/Log.h"
#include "render/Axis.h"
#include "video/Camera.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/intersect.hpp>
#include <glm/gtx/norm.hpp>

namespace render {

static constexpr float GizmoSize = 20.0f;
static constexpr glm::vec3 GizmoSizeVec(GizmoSize);

bool Gizmo::init() {
	_axis.setSize(GizmoSize, GizmoSize, GizmoSize);
	if (!_axis.init()) {
		return false;
	}

	return true;
}

void Gizmo::shutdown() {
	_axis.shutdown();
}

bool Gizmo::isMode(GizmoMode mode) const {
	return _mode == mode || _buttonMode == mode;
}

void Gizmo::render(const video::Camera& camera) {
	if (isMode(GizmoMode::TranslateX)) {
		_axis.render(camera, render::AxisMask::RenderX);
	} else if (isMode(GizmoMode::TranslateY)) {
		_axis.render(camera, render::AxisMask::RenderY);
	} else if (isMode(GizmoMode::TranslateZ)) {
		_axis.render(camera, render::AxisMask::RenderZ);
	} else {
		_axis.render(camera, render::AxisMask::RenderAll);
	}
}

GizmoMode Gizmo::mode() const {
	return _buttonMode;
}

void Gizmo::setPosition(const glm::vec3& pos) {
	_pos = pos;
	_axis.setPosition(pos);
}

void Gizmo::updateTranslateState() {
	const glm::vec3 p1 = _ray.origin;
	const glm::vec3 p2 = p1 + _ray.direction * _camera.farPlane();

	glm::vec3 paX;
	glm::vec3 pbX;
	glm::vec3 paY;
	glm::vec3 pbY;
	glm::vec3 paZ;
	glm::vec3 pbZ;

	const bool intersectX = glm::intersectLines(p1, p2, _pos, _pos + glm::vec3(GizmoSize,  0.0f,  0.0f), paX, pbX);
	const bool intersectY = glm::intersectLines(p1, p2, _pos, _pos + glm::vec3( 0.0f, GizmoSize,  0.0f), paY, pbY);
	const bool intersectZ = glm::intersectLines(p1, p2, _pos, _pos + glm::vec3( 0.0f,  0.0f, GizmoSize), paZ, pbZ);
	const float distanceX = intersectX ? glm::distance2(paX, pbX) : FLT_MAX;
	const float distanceY = intersectY ? glm::distance2(paY, pbY) : FLT_MAX;
	const float distanceZ = intersectZ ? glm::distance2(paZ, pbZ) : FLT_MAX;

	const bool smallerX = glm::any(glm::lessThan(paX, _pos));
	const bool biggerX = glm::any(glm::greaterThan(paX, _pos + GizmoSizeVec));

	const bool smallerY = glm::any(glm::lessThan(paY, _pos));
	const bool biggerY = glm::any(glm::greaterThan(paY, _pos + GizmoSizeVec));

	const bool smallerZ = glm::any(glm::lessThan(paZ, _pos));
	const bool biggerZ = glm::any(glm::greaterThan(paZ, _pos + GizmoSizeVec));

	const bool outsideX = smallerX || biggerX;
	const bool outsideY = smallerY || biggerY;
	const bool outsideZ = smallerZ || biggerZ;

	if (outsideX && outsideY && outsideZ) {
		_mode = GizmoMode::None;
		return;
	}

	const float distanceToLine = 0.3f;
	if (distanceX < distanceY && distanceX < distanceZ && distanceX < distanceToLine) {
		_mode = GizmoMode::TranslateX;
	} else if (distanceY < distanceX && distanceY < distanceZ && distanceY < distanceToLine) {
		_mode = GizmoMode::TranslateY;
	} else if (distanceZ < distanceX && distanceZ < distanceY && distanceZ < distanceToLine) {
		_mode = GizmoMode::TranslateZ;
	} else {
		_mode = GizmoMode::None;
	}
}

void Gizmo::resetMode() {
	_pixelPos = glm::ivec2(-1);
	_buttonLastPosition = glm::vec3(0.0f);
	_mode = _buttonMode = GizmoMode::None;
	_buttonLastAction = 0.0;
}

void Gizmo::updateMode(const video::Camera& camera, const glm::ivec2& pixelPos) {
	// if the cursor position didn't move, there is no need to recalculate anything
	if (_pixelPos == pixelPos) {
		return;
	}
	_pixelPos = pixelPos;
	_camera = camera;
	_ray = _camera.mouseRay(_pixelPos);
	// don't update the cached state if we have the button pressed already.
	if (!pressed()) {
		updateTranslateState();
	}
}

bool Gizmo::handleDown(int32_t key, double pressedMillis) {
	const bool initialDown = command::ActionButton::handleDown(key, pressedMillis);
	if (initialDown) {
		_buttonMode = _mode;
		_buttonLastAction = 0.0;

		// the mouse cursor position should initially not
		// contribute to the translation
		glm::vec3 delta;
		calculateTranslationDelta(delta);
	}
	return initialDown;
}

bool Gizmo::handleUp(int32_t key, double releasedMillis) {
	const bool allUp = command::ActionButton::handleUp(key, releasedMillis);
	if (!allUp) {
		return false;
	}
	if (durationSeconds < 0.15) {
		// TODO: toggle gizmo modes for translate, rotate, scale, extrude, ...
	}
	resetMode();
	return allUp;
}

bool Gizmo::execute(double nowSeconds, const std::function<void(const glm::vec3, GizmoMode)>& callback) {
	if (!pressed()) {
		return false;
	}
	// pressing into the void does nothing
	if (_buttonMode == GizmoMode::None) {
		return false;
	}
	if (nowSeconds - _buttonLastAction < 0.2) {
		return false;
	}

	glm::vec3 delta;
	if (!calculateTranslationDelta(delta)) {
		return false;
	}

	_buttonLastAction = nowSeconds;
	callback(delta, _buttonMode);
	return true;
}

bool Gizmo::calculateTranslationDelta(glm::vec3& delta) {
	glm::vec3 planeNormal;
	glm::vec3 direction;
	switch (_buttonMode) {
	case GizmoMode::TranslateX:
		planeNormal = glm::up;
		direction = glm::right;
		break;
	case GizmoMode::TranslateY:
		planeNormal = glm::left;
		direction = glm::up;
		break;
	case GizmoMode::TranslateZ:
		planeNormal = glm::up;
		direction = glm::backward;
		break;
	default:
		return false;
	}

	float len;
	if (!glm::intersectRayPlane(_ray.origin, _ray.direction, _pos, planeNormal, len)) {
		return false;
	}
	const glm::vec3 targetPos = _ray.origin + _ray.direction * glm::abs(len);
	const glm::vec3 dm = targetPos - _pos;
	const glm::vec3 rotDir = glm::conjugate(_camera.quaternion()) * direction;
	const float lengthOnAxis = glm::dot(rotDir, dm);
	const glm::vec3 moveLength = rotDir * -lengthOnAxis;
	delta = moveLength - _buttonLastPosition;
	_buttonLastPosition = moveLength;
	return true;
}

}
