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
#include <glm/gtx/closest_point.hpp>
#include <glm/gtx/intersect.hpp>
#include <glm/gtx/norm.hpp>

namespace render {

static constexpr float GizmoSize = 20.0f;
static const glm::vec3 PLANENORMALS[3] = {
	glm::backward,
	glm::up,
	glm::right
};
static const glm::vec3 DIRECTIONS[3] = {
	glm::right,
	glm::up,
	glm::backward
};

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
	_mode = GizmoMode::None;

	for (int i = 0; i < 3; i++) {
		float len;
		if (!glm::intersectRayPlane(_ray.origin, _ray.direction, _pos, PLANENORMALS[i], len)) {
			continue;
		}

		const glm::vec3 posOnPlan = _ray.origin + _ray.direction * len;
		const glm::vec2 intersectPos = _camera.worldToScreen(posOnPlan);
		const glm::vec2 start = _camera.worldToScreen(_pos);
		const glm::vec2 end = _camera.worldToScreen(_pos + DIRECTIONS[i] * GizmoSize);

		glm::vec2 pointOnAxis = glm::closestPointOnLine(intersectPos, start, end);
		if (glm::length(pointOnAxis - intersectPos) < 6.0f) {
			_mode = (GizmoMode)(core::enumVal(GizmoMode::TranslateX) + i);
			break;
		}
	}
}

bool Gizmo::calculateTranslationDelta(glm::vec3& delta) {
	if (!isTranslation()) {
		return false;
	}
	const int index = core::enumVal(_buttonMode) - core::enumVal(GizmoMode::TranslateX);
	const glm::vec3& planeNormal = PLANENORMALS[index];
	const glm::vec3& direction = DIRECTIONS[index];

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

bool Gizmo::isTranslation() const {
	return _buttonMode == GizmoMode::TranslateX || _buttonMode == GizmoMode::TranslateY || _buttonMode == GizmoMode::TranslateZ;
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

}
