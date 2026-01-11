/**
 * @file
 */

#include "Camera.h"
#include "core/Common.h"
#include "core/Assert.h"
#include "core/GLMConst.h"
#include <glm/gtc/epsilon.hpp>
#include "core/Var.h"
#include "glm/common.hpp"
#include "math/AABB.h"
#include "core/GLM.h"
#include "math/Ray.h"
#include <glm/ext/scalar_constants.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/matrix_transform.hpp>
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/compatibility.hpp>
#include <glm/gtx/norm.hpp>

namespace video {

Camera::Camera(CameraType type, CameraMode mode) :
	_type(type), _mode(mode), _quat(glm::quat_identity<float, glm::defaultp>()) {
}

float Camera::pitch() const {
	return glm::pitch(_quat);
}

float Camera::roll() const {
	return glm::roll(_quat);
}

float Camera::yaw() const {
	return glm::yaw(_quat);
}

void Camera::setYaw(float radians) {
	rotate(radians, glm::up());
}

void Camera::setRoll(float radians) {
	rotate(radians, glm::backward());
}

bool Camera::lookAt(const glm::vec3& position) {
	return lookAt(position, glm::up());
}

void Camera::turn(float radians) {
	if (fabs((double)radians) < 0.00001) {
		return;
	}
	const glm::quat& quat = glm::angleAxis(radians, _quat * glm::up());
	rotate(quat);
}

void Camera::rotate(float radians, const glm::vec3& axis) {
	if (fabs((double)radians) < 0.00001) {
		return;
	}
	const glm::quat& quat = glm::angleAxis(radians, axis);
	rotate(quat);
}

void Camera::pan(int screenDeltaX, int screenDeltaY) {
	float zoomFactor = 1.0f;
	if (_rotationType == CameraRotationType::Target) {
		if (_mode == CameraMode::Orthogonal) {
			zoomFactor = _orthoZoom / ORTHO_ZOOM_FACTOR;
		} else {
			const float rad = glm::radians(_fieldOfView);
			zoomFactor = (_distance * glm::tan(rad * 0.5f)) / (float)_windowSize.y;
		}
	}
	const glm::vec3 r = right() * ((float)-screenDeltaX) * zoomFactor;
	const glm::vec3 u = up() * ((float)screenDeltaY) * zoomFactor;
	const glm::vec3 delta = r + u;

	if (_rotationType == CameraRotationType::Target) {
		setTarget(_target + delta);
	} else {
		setWorldPosition(_worldPos + delta);
	}

	_dirty |= DIRTY_POSITION;
	_lerp = false;
}

glm::vec3 Camera::forward() const {
	return glm::conjugate(_quat) * glm::forward();
}

glm::vec3 Camera::right() const {
	return glm::conjugate(_quat) * glm::right();
}

glm::vec3 Camera::up() const {
	return glm::conjugate(_quat) * glm::up();
}

bool Camera::isOrthoAligned() const {
	return _orthoAligned;
}

void Camera::setWorldPosition(const glm::vec3& worldPos) {
	if (glm::all(glm::epsilonEqual(_worldPos, worldPos, 0.0001f))) {
		return;
	}
	_dirty |= DIRTY_POSITION;
	_worldPos = worldPos;
	if (_rotationType == CameraRotationType::Target) {
		lookAt(_target);
	}
}

glm::vec3 Camera::direction() const {
	return glm::vec3(glm::column(inverseViewMatrix(), 2));
}

void Camera::lerp(const Camera& targetCam) {
	if (&targetCam == this) {
		return;
	}
	_lerp = true;
	core_assert(!targetCam.dirty());
	_lerpTarget = {targetCam.rotationType(),
				   targetCam.target(),
				   targetCam.worldPosition(),
				   targetCam.quaternion(),
				   worldPosition(),
				   quaternion(),
				   target(),
				   0.0,
				   targetCam.targetDistance(),
				   targetDistance(),
				   targetCam.fieldOfView(),
				   fieldOfView(),
				   targetCam._orthoZoom,
				   _orthoZoom};
	setRotationType(targetCam.rotationType());
	setTarget(targetCam.target());
	setMode(targetCam.mode());
	setType(targetCam.type());
}

void Camera::setOmega(const glm::vec3& omega) {
	core_assert(!glm::any(glm::isnan(omega)));
	core_assert(!glm::any(glm::isinf(omega)));
	_omega = omega;
}

void Camera::setTarget(const glm::vec3& target) {
	core_assert(!glm::any(glm::isnan(target)));
	core_assert(!glm::any(glm::isinf(target)));
	if (glm::all(glm::epsilonEqual(_target, target, 0.0001f))) {
		return;
	}
	_dirty |= DIRTY_TARGET;
	_target = target;
}

void Camera::setAngles(float pitch, float yaw, float roll = 0.0f) {
	_quat = glm::quat(glm::vec3(pitch, yaw, roll));
	core_assert(!glm::any(glm::isnan(_quat)));
	core_assert(!glm::any(glm::isinf(_quat)));
	_dirty |= DIRTY_ORIENTATION;
	_lerp = false;
}

void Camera::setOrientation(const glm::quat& quat) {
	_quat = quat;
	core_assert(!glm::any(glm::isnan(_quat)));
	core_assert(!glm::any(glm::isinf(_quat)));
	_dirty |= DIRTY_ORIENTATION;
	_lerp = false;
}

void Camera::rotate(const glm::quat& rotation) {
	core_assert(!glm::any(glm::isnan(rotation)));
	core_assert(!glm::any(glm::isinf(rotation)));
	_quat = glm::normalize(rotation * _quat);
	_dirty |= DIRTY_ORIENTATION;
	_lerp = false;
}

void Camera::setSize(const glm::ivec2& windowSize) {
	if (_windowSize == windowSize) {
		return;
	}
	_windowSize = windowSize;
	_dirty |= DIRTY_PERSPECTIVE;
}

bool Camera::move(const glm::vec3& delta) {
	if (glm::all(glm::epsilonEqual(delta, glm::zero<glm::vec3>(), 0.0001f))) {
		return false;
	}
	_dirty |= DIRTY_POSITION;
	_worldPos += forward() * -delta.z;
	_worldPos += right() * delta.x;
	_worldPos += up() * delta.y;
	if (_rotationType == CameraRotationType::Target) {
		lookAt(_target, glm::up());
		_distance = glm::max(4.0f, glm::distance(_worldPos, _target));
		_dirty |= DIRTY_TARGET;
	}
	_lerp = false;
	return true;
}

void Camera::rotate(const glm::vec3& radians) {
	switch(_type) {
	case CameraType::FirstPerson: {
		turn(radians.y);
		setPitch(radians.x);
		break;
	}
	case CameraType::Free:
		setYaw(radians.y);
		setPitch(radians.x);
		setRoll(radians.z);
		break;
	case CameraType::UI:
	case CameraType::Max:
		break;
	}
}

float Camera::horizontalYaw() const {
	const glm::vec3& dir = direction();
	const glm::vec3 yawDirection(dir.x, 0.0f, dir.z);
	const float dotResult = glm::dot(glm::normalize(yawDirection), glm::backward());
	const float yaw = glm::acos(dotResult);
	if (yawDirection.x < 0.0f) {
		return yaw * -1.0f;
	}
	return yaw;
}

void Camera::setPitch(float radians) {
	if (_type == CameraType::FirstPerson) {
		const float dotResult = glm::dot(direction(), glm::down());
		float curPitch = glm::acos(dotResult) - glm::half_pi<float>();
		const float maxPitch = glm::half_pi<float>() - 0.1f;
		if (curPitch > maxPitch) {
			curPitch = maxPitch;
		}
		if (glm::abs(curPitch + radians) > maxPitch) {
			radians = (float)copysign((double)maxPitch, (double)curPitch) - curPitch;
		}
	}
	if (radians != 0) {
		rotate(radians, glm::right());
	}
}

void Camera::slerp(const glm::quat& quat, float factor) {
	core_assert(!glm::any(glm::isnan(quat)));
	core_assert(!glm::any(glm::isinf(quat)));
	// Ensure shortest-path interpolation: if dot < 0, negate target quaternion
	glm::quat targetQuat = quat;
	if (glm::dot(_quat, targetQuat) < 0.0f) {
		targetQuat = -targetQuat;
	}
	_quat = glm::mix(_quat, targetQuat, factor);
	_dirty |= DIRTY_ORIENTATION;
	core_assert_msg(!glm::any(glm::isnan(_quat)), "Factor: %f", factor);
	core_assert_msg(!glm::any(glm::isinf(_quat)), "Factor: %f", factor);
}

void Camera::slerp(const glm::vec3& radians, float factor) {
	const glm::quat quat2(radians);
	slerp(quat2, factor);
}

bool Camera::lookAt(const glm::vec3& position, const glm::vec3& upDirection) {
	if (glm::all(glm::epsilonEqual(_worldPos, position, 0.0001f))) {
		return false;
	}

	glm::vec3 targetDir = glm::normalize(position - _worldPos);
	if (glm::length2(targetDir) == 0) {
		targetDir = glm::forward();
	}

	glm::vec3 upDir;
	if (glm::epsilonEqual(glm::length2(upDirection), 0.0f, glm::epsilon<float>())) {
		upDir = glm::up();
	} else {
		upDir = upDirection;
	}

	if (glm::epsilonEqual(glm::length2(glm::cross(upDir, targetDir)), 0.0f, glm::epsilon<float>())) {
		upDir = glm::cross(targetDir, glm::right());
		if (glm::epsilonEqual(glm::length2(upDir), 0.0f, glm::epsilon<float>())) {
			upDir = glm::cross(targetDir, glm::backward());
		}
	}

	_quat = glm::quat_cast(glm::lookAt(_worldPos, _worldPos + targetDir, upDir));
	_dirty |= DIRTY_ORIENTATION;
	core_assert_msg(!glm::any(glm::isnan(_quat)), "upDirection(%f:%f:%f), position(%f:%f:%f), _pos(%f:%f:%f)",
			upDirection.x, upDirection.y, upDirection.z, position.x, position.y, position.z, _worldPos.x, _worldPos.y, _worldPos.z);
	core_assert_msg(!glm::any(glm::isinf(_quat)), "upDirection(%f:%f:%f), position(%f:%f:%f), _pos(%f:%f:%f)",
			upDirection.x, upDirection.y, upDirection.z, position.x, position.y, position.z, _worldPos.x, _worldPos.y, _worldPos.z);
	return true;
}

void Camera::billboard(glm::vec3 *right, glm::vec3 *up) const {
	const glm::mat4& view = viewMatrix();
	if (right != nullptr) {
		*right = glm::vec3(glm::row(view, 0));
	}
	if (up != nullptr) {
		*up = glm::vec3(glm::row(view, 1));
	}
}

void Camera::updateTarget() {
	if (_rotationType != CameraRotationType::Target) {
		return;
	}
	if (!isDirty(DIRTY_ORIENTATION | DIRTY_TARGET)) {
		return;
	}
	const glm::vec3& backward = -forward();
	const glm::vec3& newPosition = _target + backward * _distance;
	if (glm::all(glm::epsilonEqual(_worldPos, newPosition, 0.0001f))) {
		return;
	}
	_worldPos = newPosition;
	_dirty |= DIRTY_POSITION;
}

void Camera::resetZoom() {
	_orthoZoom = 1.0f;
	_distance = 100.0f;
	_dirty |= DIRTY_TARGET;
	_fieldOfView = 45.0f;
	_dirty |= DIRTY_PERSPECTIVE;
}

void Camera::zoom(float value) {
	float *target;
	if (_mode == CameraMode::Orthogonal) {
		target = &_orthoZoom;
		_dirty |= DIRTY_PERSPECTIVE;
	} else {
		target = &_distance;
		_dirty |= DIRTY_TARGET;
	}

	const float speed = core::Var::getSafe(cfg::ClientCameraZoomSpeed)->floatVal();
	const float quant = 1.0f + speed;
	if (value > 0.1f) {
		*target *= quant;
	} else  if (value < -0.1f) {
		*target /= quant;
	}

	const float maxZoom = core::Var::getSafe(cfg::ClientCameraMaxZoom)->floatVal();
	const float minZoom = core::Var::getSafe(cfg::ClientCameraMinZoom)->floatVal();
	*target = glm::clamp(*target, minZoom, maxZoom);
}

void Camera::updateLerp(double deltaFrameSeconds) {
	_lerpTarget.seconds += deltaFrameSeconds;
	const float t = glm::clamp(0.0, 1.0, _lerpTarget.seconds);
	glm::quat targetQuat = _lerpTarget.quat;
	if (glm::dot(_lerpTarget.fromQuat, targetQuat) < 0.0f) {
		targetQuat = -targetQuat;
	}
	_quat = glm::mix(_lerpTarget.fromQuat, targetQuat, t);
	_dirty |= DIRTY_ORIENTATION;
	if (_lerpTarget.rotationType == CameraRotationType::Target) {
		_target = glm::mix(_lerpTarget.fromTarget, _lerpTarget.target, t);
		_distance = glm::mix(_lerpTarget.fromDistance, _lerpTarget.distance, t);
		_dirty |= DIRTY_TARGET;
	} else {
		_worldPos = glm::mix(_lerpTarget.fromWorldPos, _lerpTarget.worldPos, t);
	}
	_fieldOfView = glm::mix(_lerpTarget.fromFieldOfView, _lerpTarget.fieldOfView, t);
	_orthoZoom = glm::mix(_lerpTarget.fromOrthoZoom, _lerpTarget.orthoZoom, t);
	_dirty |= DIRTY_POSITION | DIRTY_PERSPECTIVE;
	if (_lerpTarget.seconds > 1.0) {
		_lerp = false;
	}
}

void Camera::update(double deltaFrameSeconds) {
	if (deltaFrameSeconds > 0.0) {
		if (_lerp) {
			updateLerp(deltaFrameSeconds);
		} else {
			rotate(_omega * (float)deltaFrameSeconds);
		}
	}
	updateTarget();
	updateOrientation();
	updateViewMatrix();
	updateProjectionMatrix();
	updateFrustumPlanes();
	updateFrustumVertices();
	_viewProjectionMatrix = projectionMatrix() * viewMatrix();
	_dirty = 0u;
}

void Camera::updateOrientation() {
	if (!isDirty(DIRTY_ORIENTATION)) {
		return;
	}

	_quat = glm::normalize(_quat);
	_orientation = glm::mat4_cast(_quat);
	if (_mode == CameraMode::Orthogonal) {
		const glm::ivec3 &angles = glm::abs(glm::degrees(glm::eulerAngles(_quat)));
		const glm::ivec3 r(angles.x % 90, angles.y % 90, angles.z % 90);
		if ((r.x < 1 || r.x >= 89) && (r.y < 1 || r.y >= 89) && (r.z < 1 || r.z >= 89)) {
			_orthoAligned = true;
		} else {
			_orthoAligned = false;
		}
	} else {
		_orthoAligned = false;
	}
}

void Camera::updateProjectionMatrix() {
	if (!isDirty(DIRTY_PERSPECTIVE)) {
		return;
	}
	switch(_mode) {
	case CameraMode::Orthogonal:
		_projectionMatrix = orthogonalMatrix(nearPlane(), farPlane());
		break;
	case CameraMode::Perspective:
		_projectionMatrix = perspectiveMatrix(nearPlane(), farPlane());
		break;
	case CameraMode::Max:
		break;
	}
	_invProjectionMatrix = glm::inverse(_projectionMatrix);
}

void Camera::updateViewMatrix() {
	if (!isDirty(DIRTY_ORIENTATION | DIRTY_POSITION)) {
		return;
	}
	_viewMatrix = glm::translate(orientation(), -_worldPos);
	_invViewMatrix = glm::inverse(_viewMatrix);
}

math::Ray Camera::mouseRay(const glm::ivec2& pixelPos) const {
	const glm::vec4 viewport(0.0f, 0.0f, _windowSize.x, _windowSize.y);
	const glm::vec3 nearPlaneCoords = glm::unProject(glm::vec3(pixelPos.x, _windowSize.y - pixelPos.y, 0.0f), _viewMatrix, _projectionMatrix, viewport);
	const glm::vec3 farPlaneCoords = glm::unProject(glm::vec3(pixelPos.x, _windowSize.y - pixelPos.y, 1.0f), _viewMatrix, _projectionMatrix, viewport);
	const glm::vec3 rayDirection = glm::normalize(farPlaneCoords - nearPlaneCoords);
	return math::Ray(nearPlaneCoords, rayDirection);
}

glm::ivec2 Camera::worldToScreen(const glm::vec3& worldPos) const {
	return worldToScreen(viewProjectionMatrix(), worldPos);
}

glm::ivec2 Camera::worldToScreen(const glm::mat4& matrix, const glm::vec3& worldPos) const {
	glm::vec4 trans = matrix * glm::vec4(worldPos, 1.0f);
	trans *= 0.5f / trans.w;
	trans += glm::vec4(0.5f, 0.5f, 0.0f, 0.0f);
	trans.y = 1.0f - trans.y;
	trans.x *= (float)_windowSize.x;
	trans.y *= (float)_windowSize.y;
	return glm::ivec2(trans.x, trans.y);
}

/**
 * http://developer.download.nvidia.com/SDK/10.5/opengl/src/cascaded_shadow_maps/doc/cascaded_shadow_maps.pdf
 */
void Camera::sliceFrustum(float* sliceBuf, int bufSize, int splits, float sliceWeight) const {
	core_assert_always(bufSize >= splits * 2);
	core_assert_always(splits >= 1);
	const float near = nearPlane();
	const float far = farPlane();
	const float ratio = far / (glm::abs(near) + glm::epsilon<float>());
#if 1
	const int numSlices = splits * 2;
	const float splitsf = (float)numSlices;
	const float dist = far - near;
	const float sliceWeightInv = 1.0f - sliceWeight;

	sliceBuf[0] = near;
	sliceBuf[numSlices - 1] = far;

	for (int nearIdx = 2, farIdx = 1; nearIdx < numSlices; farIdx += 2, nearIdx += 2) {
		const float si = (float)farIdx / splitsf;
		const float nearp = sliceWeight * (near * glm::pow(ratio, si)) + sliceWeightInv * (near + dist * si);
		sliceBuf[nearIdx] = nearp;
		sliceBuf[farIdx] = nearp * 1.005f;
	}
#else
	int bufIdx = 0;
	for (int split = 0; split < splits; ++split) {
		const float nearK = float(bufIdx) / splits;
		const float nearLogd = near * glm::pow(ratio, nearK);
		const float nearLind = glm::mix(near, far, nearK);
		const float nearSplitVal = glm::mix(nearLogd, nearLind, sliceWeight);
		sliceBuf[bufIdx++] = nearSplitVal;

		const float farK = float(bufIdx) / splits;
		const float farLogd = near * glm::pow(ratio, farK);
		const float farLind = glm::mix(near, far, farK);
		const float farSplitVal = glm::mix(farLogd, farLind, sliceWeight);
		sliceBuf[bufIdx++] = farSplitVal;
	}
#endif
}

void Camera::splitFrustum(float nearPlane, float farPlane, glm::vec3 out[math::FRUSTUM_VERTICES_MAX]) const {
	glm::mat4 proj;
	switch(_mode) {
	case CameraMode::Orthogonal:
		proj = orthogonalMatrix(nearPlane, farPlane);
		break;
	case CameraMode::Perspective:
	default:
		proj = perspectiveMatrix(nearPlane, farPlane);
		break;
	}

	const glm::mat4& transform = glm::inverse(proj * viewMatrix());
	frustum().split(transform, out);
}

void Camera::updateFrustumVertices() {
	if (!isDirty(DIRTY_ORIENTATION | DIRTY_POSITION | DIRTY_PERSPECTIVE)) {
		return;
	}

	_frustum.updateVertices(viewMatrix(), projectionMatrix());
}

void Camera::updateFrustumPlanes() {
	if (!isDirty(DIRTY_ORIENTATION | DIRTY_POSITION | DIRTY_PERSPECTIVE)) {
		return;
	}

	_frustum.updatePlanes(viewMatrix(), projectionMatrix());
}

math::AABB<float> Camera::aabb() const {
	core_assert(!isDirty(DIRTY_ORIENTATION | DIRTY_POSITION | DIRTY_PERSPECTIVE));
	return frustum().aabb();
}

glm::vec4 Camera::sphereBoundingBox() const {
	const math::AABB<float> boundingBox = aabb();
	const glm::vec3& mins = boundingBox.getLowerCorner();
	const glm::vec3& maxs = boundingBox.getUpperCorner();

	const glm::vec3 sphereCenter(
			mins.x + (maxs.x - mins.x) / 2.0f,
			mins.y + (maxs.x - mins.y) / 2.0f,
			mins.z + (maxs.z - mins.z) / 2.0f);
	const float sphereRadius = core_max(core_max((maxs.x - mins.x) / 2.0f, (maxs.y - mins.y) / 2.0f), (maxs.z - mins.z) / 2.0f);

	return glm::vec4(sphereCenter, sphereRadius);
}

glm::mat4 Camera::orthogonalMatrix(float nplane, float fplane) const {
	if (_type == CameraType::UI) {
		const float left = 0.0f;
		const float top = 0.0f;
		const float right = (float)_windowSize.x;
		const float bottom = (float)_windowSize.y;
		core_assert_msg(right > left, "Invalid dimension given: right must be greater than left but is %f", right);
		core_assert_msg(top < bottom, "Invalid dimension given: top must be smaller than bottom but is %f", top);
		return glm::ortho(left, right, bottom, top, nplane, fplane);
	}
	const float zoom = _orthoZoom / ORTHO_ZOOM_FACTOR;
	const float halfWidth = (float)_windowSize.x / 2.0f;
	const float halfHeight = (float)_windowSize.y / 2.0f;
	const float left = -halfWidth;
	const float right = halfWidth;
	const float bottom = -halfHeight;
	const float top = halfHeight;
	return glm::orthoRH_NO(left * zoom, right * zoom, bottom * zoom, top * zoom, nplane, fplane);
}

glm::mat4 Camera::perspectiveMatrix(float nplane, float fplane) const {
	const float fov = glm::radians(_fieldOfView);
	return glm::perspectiveFovRH_NO(fov, (float)_windowSize.x, (float)_windowSize.y, nplane, fplane);
}

void Camera::setNearPlane(float nearPlane) {
	if (glm::epsilonEqual(_nearPlane, nearPlane, 0.00001f)) {
		return;
	}

	_dirty |= DIRTY_PERSPECTIVE;
	if (_mode == CameraMode::Orthogonal) {
		_nearPlane = nearPlane;
	} else {
		_nearPlane = core_max(0.1f, nearPlane);
	}
}

void Camera::setFarPlane(float farPlane) {
	if (glm::epsilonEqual(_farPlane, farPlane, 0.00001f)) {
		return;
	}

	_dirty |= DIRTY_PERSPECTIVE;
	_farPlane = farPlane;
}

bool Camera::isVisible(const math::AABB<float>& aabb) const {
	return isVisible(aabb.getLowerCorner(), aabb.getUpperCorner());
}

bool Camera::operator==(const Camera &other) const {
	return _type == other._type && _mode == other._mode && _rotationType == other._rotationType &&
		   _windowSize == other._windowSize && glm::epsilonEqual(_distance, other._distance, 0.0001f) &&
		   glm::epsilonEqual(_fieldOfView, other._fieldOfView, 0.0001f) &&
		   glm::epsilonEqual(_orthoZoom, other._orthoZoom, 0.0001f) &&
		   glm::all(glm::epsilonEqual(_worldPos, other._worldPos, 0.0001f)) &&
		   glm::all(glm::epsilonEqual(_target, other._target, 0.0001f)) &&
		   glm::all(glm::epsilonEqual(_quat, other._quat, 0.0001f));
}

}
