/**
 * @file
 */

#include "Camera.h"
#include "core/Common.h"
#include "core/Assert.h"
#include "core/Singleton.h"
#include "math/AABB.h"
#include "Ray.h"
#include <glm/gtc/matrix_access.hpp>
#include <algorithm>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

namespace video {

Camera::Camera(CameraType type, CameraMode mode) :
	_type(type), _mode(mode), _pos(glm::vec3(0.0f)), _omega(0.0f) {
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

void Camera::yaw(float radians) {
	rotate(radians, glm::up);
}

void Camera::turn(float radians) {
	if (fabs(radians) < 0.00001f) {
		return;
	}
	const glm::quat& quat = glm::angleAxis(radians, _quat * glm::up);
	rotate(quat);
}

void Camera::rotate(float radians, const glm::vec3& axis) {
	if (fabs(radians) < 0.00001f) {
		return;
	}
	const glm::quat& quat = glm::angleAxis(radians, axis);
	rotate(quat);
}

glm::vec3 Camera::forward() const {
	return glm::conjugate(_quat) * glm::forward;
}

glm::vec3 Camera::right() const {
	return glm::conjugate(_quat) * glm::right;
}

glm::vec3 Camera::up() const {
	return glm::conjugate(_quat) * glm::up;
}

void Camera::setPosition(const glm::vec3& pos) {
	if (glm::all(glm::epsilonEqual(_pos, pos, 0.0001f))) {
		return;
	}
	_dirty |= DIRTY_POSITON;
	_pos = pos;
	if (_rotationType == CameraRotationType::Target) {
		lookAt(_target);
	}
}

glm::vec3 Camera::direction() const {
	return glm::vec3(glm::column(inverseViewMatrix(), 2));
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
}

void Camera::setQuaternion(const glm::quat& quat) {
	_quat = quat;
	core_assert(!glm::any(glm::isnan(_quat)));
	core_assert(!glm::any(glm::isinf(_quat)));
	_dirty |= DIRTY_ORIENTATION;
}

void Camera::rotate(const glm::quat& rotation) {
	core_assert(!glm::any(glm::isnan(rotation)));
	core_assert(!glm::any(glm::isinf(rotation)));
	_quat = glm::normalize(rotation * _quat);
	_dirty |= DIRTY_ORIENTATION;
}

void Camera::init(const glm::ivec2& position, const glm::ivec2& frameBufferSize, const glm::ivec2& windowSize) {
	if (_position == position && _frameBufferSize == frameBufferSize && _windowSize == windowSize) {
		return;
	}
	_position = position;
	_frameBufferSize = frameBufferSize;
	_windowSize = windowSize;
	_frameBufferAspectRatio = _frameBufferSize.x / static_cast<float>(_frameBufferSize.y);
	_dirty = DIRTY_ALL;
}

void Camera::move(const glm::vec3& delta) {
	if (glm::all(glm::epsilonEqual(delta, glm::zero<glm::vec3>(), 0.0001f))) {
		return;
	}
	_dirty |= DIRTY_POSITON;
	_pos += forward() * -delta.z;
	_pos += right() * delta.x;
	_pos += up() * delta.y;
	if (_rotationType == CameraRotationType::Target) {
		lookAt(_target, glm::up);
		_dirty |= DIRTY_TARGET;
	}
}

void Camera::rotate(const glm::vec3& radians) {
	switch(_type) {
	case CameraType::FirstPerson: {
		turn(radians.y);
		pitch(radians.x);
		break;
	}
	case CameraType::Free:
		yaw(radians.y);
		pitch(radians.x);
		roll(radians.z);
		break;
	}
}

float Camera::horizontalYaw() const {
	const glm::vec3& dir = direction();
	const glm::vec3 yawDirection(dir.x, 0.0f, dir.z);
	const float dotResult = glm::dot(glm::normalize(yawDirection), glm::backward);
	const float yaw = glm::acos(dotResult);
	if (yawDirection.x < 0.0f) {
		return yaw * -1.0f;
	}
	return yaw;
}

void Camera::pitch(float radians) {
	if (_type == CameraType::FirstPerson) {
		const float dotResult = glm::dot(direction(), glm::down);
		float curpitch = glm::acos(dotResult) - glm::half_pi<float>();
		if (curpitch > MAX_PITCH) {
			curpitch = MAX_PITCH;
		}
		if (glm::abs(curpitch + radians) > MAX_PITCH) {
			radians = copysign(MAX_PITCH, curpitch) - curpitch;
		}
	}
	if (radians != 0) {
		rotate(radians, glm::right);
	}
}

inline void Camera::slerp(const glm::quat& quat, float factor) {
	_quat = glm::mix(_quat, quat, factor);
	_dirty |= DIRTY_ORIENTATION;
	core_assert(!glm::any(glm::isnan(_quat)));
	core_assert(!glm::any(glm::isinf(_quat)));
}

void Camera::slerp(const glm::vec3& radians, float factor) {
	const glm::quat quat2(radians);
	_quat = glm::mix(_quat, quat2, factor);
	_dirty |= DIRTY_ORIENTATION;
	core_assert(!glm::any(glm::isnan(_quat)));
	core_assert(!glm::any(glm::isinf(_quat)));
}

bool Camera::lookAt(const glm::vec3& position, const glm::vec3& upDirection) {
	if (glm::all(glm::epsilonEqual(_pos, position, 0.0001f))) {
		return false;
	}

	glm::vec3 targetDir = glm::normalize(position - _pos);
	if (glm::length2(targetDir) == 0) {
		targetDir = glm::forward;
	}

	glm::vec3 upDir;
	if (glm::epsilonEqual(glm::length2(upDirection), 0.0f, glm::epsilon<float>())) {
		upDir = glm::up;
	} else {
		upDir = upDirection;
	}

	if (glm::epsilonEqual(glm::length2(glm::cross(upDir, targetDir)), 0.0f, glm::epsilon<float>())) {
		upDir = glm::cross(targetDir, glm::right);
		if (glm::epsilonEqual(glm::length2(upDir), 0.0f, glm::epsilon<float>())) {
			upDir = glm::cross(targetDir, glm::backward);
		}
	}

	_quat = glm::quat_cast(glm::lookAt(_pos, _pos + targetDir, upDir));
	_dirty |= DIRTY_ORIENTATION;
	core_assert_msg(!glm::any(glm::isnan(_quat)), "upDirection(%f:%f:%f), position(%f:%f:%f), _pos(%f:%f:%f)",
			upDirection.x, upDirection.y, upDirection.z, position.x, position.y, position.z, _pos.x, _pos.y, _pos.z);
	core_assert_msg(!glm::any(glm::isinf(_quat)), "upDirection(%f:%f:%f), position(%f:%f:%f), _pos(%f:%f:%f)",
			upDirection.x, upDirection.y, upDirection.z, position.x, position.y, position.z, _pos.x, _pos.y, _pos.z);
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
	if (glm::all(glm::epsilonEqual(_pos, newPosition, 0.0001f))) {
		return;
	}
	_pos = newPosition;
	_dirty |= DIRTY_POSITON;
}

void Camera::update(uint64_t deltaFrame) {
	if (deltaFrame > 0) {
		const float dt = deltaFrame / 1000.0f;
		rotate(_omega * dt);
	}
	updateTarget();
	updateOrientation();
	updateViewMatrix();
	updateProjectionMatrix();
	updateFrustumPlanes();
	updateFrustumVertices();
	_viewProjectionMatrix = projectionMatrix() * viewMatrix();
	_dirty = 0;
}

void Camera::updateOrientation() {
	if (!isDirty(DIRTY_ORIENTATION)) {
		return;
	}

	_quat = glm::normalize(_quat);
	_orientation = glm::mat4_cast(_quat);
}

void Camera::updateProjectionMatrix() {
	if (!isDirty(DIRTY_PERSPECTIVE)) {
		return;
	}
	switch(_mode) {
	case CameraMode::Orthogonal:
		_projectionMatrix = orthogonalMatrix();
		break;
	case CameraMode::Perspective:
		_projectionMatrix = perspectiveMatrix();
		break;
	}
	_invProjectionMatrix = glm::inverse(_projectionMatrix);
}

void Camera::updateViewMatrix() {
	if (!isDirty(DIRTY_ORIENTATION | DIRTY_POSITON)) {
		return;
	}
	_viewMatrix = glm::translate(orientation(), -_pos);
	_invViewMatrix = glm::inverse(_viewMatrix);
	_eyePosition =_invViewMatrix[3];
}

Ray Camera::mouseRay(const glm::ivec2& pixelPos) const {
	return screenRay(glm::vec2(pixelPos.x / (float)_frameBufferSize.x, pixelPos.y / (float)_frameBufferSize.y));
	/*const glm::vec2 newPos = glm::vec2(screenPos - _position) / glm::vec2(dimension());
	return screenRay(newPos);*/
}

Ray Camera::screenRay(const glm::vec2& screenPos) const {
	// project screen position [0.0-1.0] to [-1.0,1.0] and flip y axis
	// to bring them into homogeneous clip coordinates
	const float x = (2.0f * screenPos.x) - 1.0f;
	const float y = 1.0f - (2.0f * screenPos.y);
	const glm::vec4 rayClipSpace(x, y, -1.0f, 1.0f);

	glm::vec4 rayEyeSpace = inverseProjectionMatrix() * rayClipSpace;
	rayEyeSpace.z = -1.0f;
	rayEyeSpace.w = 0.0f;

	const glm::vec3& rayDirection = glm::normalize(glm::vec3(inverseViewMatrix() * rayEyeSpace));
	return Ray(position(), rayDirection);
}

glm::vec3 Camera::screenToWorld(const glm::vec3& screenPos) const {
	const Ray& ray = screenRay(glm::vec2(screenPos));
	return ray.origin + ray.direction * screenPos.z;
}

/**
 * http://developer.download.nvidia.com/SDK/10.5/opengl/src/cascaded_shadow_maps/doc/cascaded_shadow_maps.pdf
 */
void Camera::sliceFrustum(float* sliceBuf, int bufSize, int splits, float sliceWeight) const {
	core_assert_always(bufSize >= splits * 2);
	core_assert_always(splits >= 1);
	const float near = nearPlane();
	const float far = farPlane();
	const float ratio = far / near;
#if 1
	const int8_t numSlices = splits * 2;
	const float splitsf = (float)numSlices;
	const float dist = far - near;
	const float sliceWeightInv = 1.0f - sliceWeight;

	sliceBuf[0] = near;
	sliceBuf[numSlices - 1] = far;

	for (uint8_t farIdx = 2, nearIdx = 1; farIdx < numSlices; farIdx += 2, nearIdx += 2) {
		const float si = float(int8_t(nearIdx)) / splitsf;
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
		proj = glm::ortho(0.0f, (float)_windowSize.x, (float)_windowSize.y, 0.0f, nearPlane, farPlane);
		break;
	case CameraMode::Perspective:
	default:
		proj = glm::perspective(glm::radians(_fieldOfView), _frameBufferAspectRatio, nearPlane, farPlane);
		break;
	}

	const glm::mat4& transform = glm::inverse(proj * viewMatrix());
	frustum().split(transform, out);
}

void Camera::updateFrustumVertices() {
	if (!isDirty(DIRTY_ORIENTATION | DIRTY_POSITON | DIRTY_PERSPECTIVE)) {
		return;
	}

	_frustum.updateVertices(viewMatrix(), projectionMatrix());
}

void Camera::updateFrustumPlanes() {
	if (!isDirty(DIRTY_ORIENTATION | DIRTY_POSITON | DIRTY_PERSPECTIVE)) {
		return;
	}

	_frustum.updatePlanes(viewMatrix(), projectionMatrix());
}

math::AABB<float> Camera::aabb() const {
	core_assert(!isDirty(DIRTY_ORIENTATION | DIRTY_POSITON | DIRTY_PERSPECTIVE));
	return frustum().aabb();
}

static inline float getBoundingSphereRadius(const glm::vec3& center, const std::vector<glm::vec3>& points) {
	float radius = 0.0f;
	for (const glm::vec3& p : points) {
		radius = core_max(radius, glm::distance(center, p));
	}
	return radius;
}

glm::vec4 Camera::splitFrustumSphereBoundingBox(float near, float far) const {
	const glm::mat4& projection = projectionMatrix();
	const glm::mat4& inverseProjection = inverseProjectionMatrix();

	const float znearp = glm::project(projection, glm::vec3(0.0f, 0.0f, -near)).z;
	const float zfarp = glm::project(projection, glm::vec3(0.0f, 0.0f, -far)).z;

	std::vector<glm::vec3> points;
	points.reserve(8);

	for (int x = 0; x < 2; ++x) {
		for (int y = 0; y < 2; ++y) {
			for (int z = 0; z < 2; ++z) {
				const glm::vec3 v(x ? 1 : -1, y ? 1 : -1, z ? zfarp : znearp);
				const glm::vec3& p = glm::project(inverseProjection, v);
				points.emplace_back(p);
			}
		}
	}

	const glm::vec3& begin = glm::project(inverseProjection, glm::vec3(0.0f, 0.0f, znearp));
	const glm::vec3& end = glm::project(inverseProjection, glm::vec3(0.0f, 0.0f, zfarp));
	float radiusBegin = getBoundingSphereRadius(begin, points);
	float radiusEnd = getBoundingSphereRadius(end, points);

	float rangeBegin = 0.0f;
	float rangeEnd = 1.0f;

	while (rangeEnd - rangeBegin > 1e-3) {
		const float rangeMiddle = (rangeBegin + rangeEnd) / 2.0f;
		const float radiusMiddle = getBoundingSphereRadius(glm::mix(begin, end, rangeMiddle), points);

		if (radiusBegin < radiusEnd) {
			radiusEnd = radiusMiddle;
			rangeEnd = rangeMiddle;
		} else {
			radiusBegin = radiusMiddle;
			rangeBegin = rangeMiddle;
		}
	}

	return glm::vec4(glm::mix(begin, end, rangeBegin), radiusBegin);
}

glm::vec4 Camera::sphereBoundingBox() const {
	const math::AABB<float> boundingBox = aabb();
	const glm::vec3& mins = boundingBox.getLowerCorner();
	const glm::vec3& maxs = boundingBox.getUpperCorner();

	const glm::vec3 sphereCenter(
			mins.x + (maxs.x - mins.x) / 2.0f,
			mins.y + (maxs.x - mins.y) / 2.0f,
			mins.z + (maxs.z - mins.z) / 2.0f);
	const float sphereRadius = (std::max)({
			(maxs.x - mins.x) / 2.0f,
			(maxs.y - mins.y) / 2.0f,
			(maxs.z - mins.z) / 2.0f});

	return glm::vec4(sphereCenter, sphereRadius);
}

glm::mat4 Camera::orthogonalMatrix() const {
	const float left = x();
	const float bottom = y();
	const float right = left + _windowSize.x;
	const float top = bottom + _windowSize.y;
	core_assert_msg(right > left, "Invalid dimension given: right must be greater than left but is %f", right);
	core_assert_msg(top > bottom, "Invalid dimension given: top must be greater than bottom but is %f", top);
	return glm::ortho(left, right, top, bottom, nearPlane(), farPlane());
}

glm::mat4 Camera::perspectiveMatrix() const {
	return glm::perspectiveFovRH_NO(glm::radians(_fieldOfView), (float)_windowSize.x, (float)_windowSize.y, nearPlane(), farPlane());
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

}
