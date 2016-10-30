/**
 * @file
 */

#include "Camera.h"
#include "core/Common.h"
#include "core/Singleton.h"
#include "io/EventHandler.h"
#include "core/GLM.h"

namespace video {

Camera::Camera(CameraType type, CameraMode mode) :
	_type(type), _mode(mode), _pos(glm::vec3()), _omega(0.0f) {
}

Camera::~Camera() {
}

void Camera::init(const glm::ivec2& position, const glm::ivec2& dimension) {
	_position = position;
	_dimension = dimension;
	_aspectRatio = _dimension.x / static_cast<float>(_dimension.y);
	_dirty = DIRTY_ALL;
}

void Camera::move(const glm::vec3& delta) {
	if (glm::all(glm::epsilonEqual(delta, glm::vec3(), 0.0001f))) {
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
	// TODO: what about rotationtype... should matter here, too, no?
	switch(_type) {
	case CameraType::FirstPerson:
		turn(radians.y);
		pitch(radians.x);
		break;
	case CameraType::Free:
		yaw(radians.y);
		pitch(radians.x);
		roll(radians.z);
		break;
	}
	_dirty |= DIRTY_ORIENTATION;
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

void Camera::lookAt(const glm::vec3& position, const glm::vec3& upDirection) {
	core_assert(position != _pos);
	_quat = glm::quat_cast(glm::lookAt(_pos, position, upDirection));
	_dirty |= DIRTY_ORIENTATION;
	core_assert(!glm::any(glm::isnan(_quat)));
	core_assert(!glm::any(glm::isinf(_quat)));
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

void Camera::update(long deltaFrame) {
	const float dt = deltaFrame / 1000.0f;
	rotate(_omega * dt);
	updateTarget();
	updateOrientation();
	updateViewMatrix();
	updateProjectionMatrix();
	updateFrustumPlanes();
	updateFrustumVertices();
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
}

void Camera::updateViewMatrix() {
	if (!isDirty(DIRTY_ORIENTATION | DIRTY_POSITON)) {
		return;
	}
	_viewMatrix = glm::translate(orientation(), -_pos);
}

Ray Camera::mouseRay(const glm::ivec2& screenPos) const {
	return screenRay(glm::vec2(screenPos.x / (float)width(), screenPos.y / (float)height()));
	/*const glm::vec2 newPos = glm::vec2(screenPos - _position) / glm::vec2(dimension());
	return screenRay(newPos);*/
}

Ray Camera::screenRay(const glm::vec2& screenPos) const {
	// project screen position [0.0-1.0] to [-1.0,1.0] and flip y axis
	// to bring them into homogeneous clip coordinates
	const float x = (2.0f * screenPos.x) - 1.0f;
	const float y = 1.0f - (2.0f * screenPos.y);
	const glm::vec4 rayClipSpace(x, y, -1.0f, 1.0f);

	glm::vec4 rayEyeSpace = glm::inverse(projectionMatrix()) * rayClipSpace;
	rayEyeSpace = glm::vec4(rayEyeSpace.xy(), -1.0f, 0.0f);

	const glm::vec3& rayDirection = glm::normalize((glm::inverse(viewMatrix()) * rayEyeSpace).xyz());
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
	const int numSlices = splits * 2;
	const float numSlicesf = float(numSlices);
	const float ratio = far / near;
	const float delta = far - near;

	sliceBuf[0] = near;
	sliceBuf[numSlices - 1] = far;

	for (int farIndex = 1, nearIndex = 2; nearIndex < numSlices; farIndex += 2, nearIndex += 2) {
		const float exponent = farIndex / numSlicesf;
		const float one = sliceWeight * (near * glm::pow(ratio, exponent));
		const float two = (1 - sliceWeight) * (near + delta * exponent);
		const float nearPlaneSlice = one + two;
		sliceBuf[nearIndex] = nearPlaneSlice;
		sliceBuf[farIndex] = nearPlaneSlice * 1.005f;
	}
}

void Camera::splitFrustum(float nearPlane, float farPlane, glm::vec3 out[core::FRUSTUM_VERTICES_MAX]) const {
	glm::mat4 proj(glm::uninitialize);
	switch(_mode) {
	case CameraMode::Orthogonal:
		proj = glm::ortho(0.0f, (float)width(), (float)height(), 0.0f, nearPlane, farPlane);
		break;
	case CameraMode::Perspective:
		proj = glm::perspective(glm::radians(_fieldOfView), _aspectRatio, nearPlane, farPlane);
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

core::AABB<float> Camera::aabb() const {
	core_assert(!isDirty(DIRTY_ORIENTATION | DIRTY_POSITON | DIRTY_PERSPECTIVE));
	return frustum().aabb();
}

glm::vec4 Camera::sphereBoundingBox() const {
	const core::AABB<float> boundingBox = aabb();
	const glm::vec3& mins = boundingBox.getLowerCorner();
	const glm::vec3& maxs = boundingBox.getUpperCorner();

	const glm::vec3 sphereCenter(
			mins.x + (maxs.x - mins.x) / 2.0f,
			mins.y + (maxs.x - mins.y) / 2.0f,
			mins.z + (maxs.z - mins.z) / 2.0f);
	const float sphereRadius = std::max({
			(maxs.x - mins.x) / 2.0f,
			(maxs.y - mins.y) / 2.0f,
			(maxs.z - mins.z) / 2.0f});

	return glm::vec4(sphereCenter, sphereRadius);
}

}
