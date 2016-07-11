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
	_type(type), _mode(mode), _pos(glm::zero<float>()), _omega(0.0f) {
	_dirty |= DIRTY_ORIENTATION;
}

Camera::~Camera() {
}

void Camera::init(int width, int height) {
	_width = width;
	_height = height;
}

void Camera::move(const glm::vec3& delta) {
	_pos += forward() * -delta.z;
	_pos += right() * delta.x;
	_pos += up() * delta.y;
	_dirty |= DIRTY_POSITON;
}

void Camera::rotate(int32_t deltaX, int32_t deltaY, float rotationSpeed) {
	switch(_type) {
		case CameraType::FirstPerson:
			turn(deltaX * rotationSpeed);
			break;
		case CameraType::Free:
			yaw(deltaX * rotationSpeed);
			break;
	}
	pitch(deltaY * rotationSpeed);
	_dirty |= DIRTY_ORIENTATION;
}

inline void Camera::slerp(const glm::quat& quat, float factor) {
	_quat = glm::mix(_quat, quat, factor);
	_dirty |= DIRTY_ORIENTATION;
}

inline void Camera::slerp(float pitch, float yaw, float factor) {
	slerp(pitch, yaw, 0.0f, factor);
}

void Camera::slerp(float pitch, float yaw, float roll, float factor) {
	const glm::quat quat2(glm::vec3(pitch, yaw, roll));
	_quat = glm::mix(_quat, quat2, factor);
	_dirty |= DIRTY_ORIENTATION;
}

void Camera::lookAt(const glm::vec3& position, const glm::vec3& upDirection) {
	core_assert(position != _pos);
	_dirty |= DIRTY_ORIENTATION;
	_quat = glm::quat_cast(glm::lookAt(_pos, position, upDirection));
	core_assert(!glm::any(glm::isnan(glm::vec4(_quat.x, _quat.y, _quat.z, _quat.w))));
}

void Camera::updateTarget() {
	if (_rotationType != CameraRotationType::Target) {
		return;
	}
	if (!isDirty(DIRTY_ORIENTATION | DIRTY_TARGET)) {
		return;
	}
	const glm::vec3& backward = -forward();
	setPosition(_target + backward * _distance);
}

void Camera::update(long deltaFrame) {
	updateTarget();
	updateOrientation();
	updateViewMatrix();
	updateProjectionMatrix();
	updateFrustumPlanes();
	// TODO: apply omega rotation
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

Ray Camera::screenRay(const glm::vec2& screenPos) const {
	// project relative mouse cursor position [0.0-1.0] to [-1.0,1.0] and flip y axis
	const float x = +(screenPos.x - 0.5f) * 2.0f;
	const float y = -(screenPos.y - 0.5f) * 2.0f;
	const glm::mat4& viewProjInverse = glm::inverse(_projectionMatrix * _viewMatrix);
	const glm::vec4 near(x, y, 0.0f, 1.0f);
	const glm::vec4 far(x, y, 1.0f, 1.0f);
	const glm::vec4& origin = viewProjInverse * near;
	return Ray(glm::vec3(origin), glm::normalize(glm::vec3(viewProjInverse * far - origin)));
}

glm::vec3 Camera::screenToWorld(const glm::vec3& screenPos) const {
	const Ray& ray = screenRay(glm::vec2(screenPos));
	return ray.origin + ray.direction * screenPos.z;
}

FrustumResult Camera::testFrustum(const glm::vec3& position) const {
	FrustumResult result = FrustumResult::Inside;
	for (int i = 0; i < int(FrustumPlanes::MaxPlanes); i++) {
		const glm::vec3 normal(_frustumPlanes[i]);
		const float pos = _frustumPlanes[i].w;
		if (glm::dot(normal, position) + pos < 0.0f) {
			return FrustumResult::Outside;
		}
	}
	return result;
}

FrustumResult Camera::testFrustum(const glm::vec3& mins, const glm::vec3& maxs) const {
	FrustumResult result = FrustumResult::Inside;
	for (uint i = 0; i < int(FrustumPlanes::MaxPlanes); i++) {
		const float pos = _frustumPlanes[i].w;
		const glm::vec3 normal(_frustumPlanes[i]);

		glm::vec3 positiveVertex = mins;
		if (normal.x >= 0.0f)
			positiveVertex.x = maxs.x;
		if (normal.y >= 0.0f)
			positiveVertex.y = maxs.y;
		if (normal.z >= 0.0f)
			positiveVertex.z = maxs.z;

		if (glm::dot(normal, positiveVertex) + pos < 0.0f) {
			return FrustumResult::Outside;
		}

		glm::vec3 negativeVertex = maxs;
		if (normal.x >= 0.0f)
			negativeVertex.x = mins.x;
		if (normal.y >= 0.0f)
			negativeVertex.y = mins.y;
		if (normal.z >= 0.0f)
			negativeVertex.z = mins.z;

		if (glm::dot(normal, negativeVertex) + pos < 0.0f) {
			result = FrustumResult::Intersect;
		}
	}

	return result;
}

void Camera::updateFrustumPlanes() {
	const glm::mat4 &v = _viewMatrix;
	const glm::mat4 &p = _projectionMatrix;

	glm::mat4 clipMatrix;

	clipMatrix[0][0] = v[0][0] * p[0][0] + v[0][1] * p[1][0] + v[0][2] * p[2][0] + v[0][3] * p[3][0];
	clipMatrix[1][0] = v[0][0] * p[0][1] + v[0][1] * p[1][1] + v[0][2] * p[2][1] + v[0][3] * p[3][1];
	clipMatrix[2][0] = v[0][0] * p[0][2] + v[0][1] * p[1][2] + v[0][2] * p[2][2] + v[0][3] * p[3][2];
	clipMatrix[3][0] = v[0][0] * p[0][3] + v[0][1] * p[1][3] + v[0][2] * p[2][3] + v[0][3] * p[3][3];
	clipMatrix[0][1] = v[1][0] * p[0][0] + v[1][1] * p[1][0] + v[1][2] * p[2][0] + v[1][3] * p[3][0];
	clipMatrix[1][1] = v[1][0] * p[0][1] + v[1][1] * p[1][1] + v[1][2] * p[2][1] + v[1][3] * p[3][1];
	clipMatrix[2][1] = v[1][0] * p[0][2] + v[1][1] * p[1][2] + v[1][2] * p[2][2] + v[1][3] * p[3][2];
	clipMatrix[3][1] = v[1][0] * p[0][3] + v[1][1] * p[1][3] + v[1][2] * p[2][3] + v[1][3] * p[3][3];
	clipMatrix[0][2] = v[2][0] * p[0][0] + v[2][1] * p[1][0] + v[2][2] * p[2][0] + v[2][3] * p[3][0];
	clipMatrix[1][2] = v[2][0] * p[0][1] + v[2][1] * p[1][1] + v[2][2] * p[2][1] + v[2][3] * p[3][1];
	clipMatrix[2][2] = v[2][0] * p[0][2] + v[2][1] * p[1][2] + v[2][2] * p[2][2] + v[2][3] * p[3][2];
	clipMatrix[3][2] = v[2][0] * p[0][3] + v[2][1] * p[1][3] + v[2][2] * p[2][3] + v[2][3] * p[3][3];
	clipMatrix[0][3] = v[3][0] * p[0][0] + v[3][1] * p[1][0] + v[3][2] * p[2][0] + v[3][3] * p[3][0];
	clipMatrix[1][3] = v[3][0] * p[0][1] + v[3][1] * p[1][1] + v[3][2] * p[2][1] + v[3][3] * p[3][1];
	clipMatrix[2][3] = v[3][0] * p[0][2] + v[3][1] * p[1][2] + v[3][2] * p[2][2] + v[3][3] * p[3][2];
	clipMatrix[3][3] = v[3][0] * p[0][3] + v[3][1] * p[1][3] + v[3][2] * p[2][3] + v[3][3] * p[3][3];

	_frustumPlanes[int(FrustumPlanes::FrustumRight)].x = clipMatrix[3][0] - clipMatrix[0][0];
	_frustumPlanes[int(FrustumPlanes::FrustumRight)].y = clipMatrix[3][1] - clipMatrix[0][1];
	_frustumPlanes[int(FrustumPlanes::FrustumRight)].z = clipMatrix[3][2] - clipMatrix[0][2];
	_frustumPlanes[int(FrustumPlanes::FrustumRight)].w = clipMatrix[3][3] - clipMatrix[0][3];

	_frustumPlanes[int(FrustumPlanes::FrustumLeft)].x = clipMatrix[3][0] + clipMatrix[0][0];
	_frustumPlanes[int(FrustumPlanes::FrustumLeft)].y = clipMatrix[3][1] + clipMatrix[0][1];
	_frustumPlanes[int(FrustumPlanes::FrustumLeft)].z = clipMatrix[3][2] + clipMatrix[0][2];
	_frustumPlanes[int(FrustumPlanes::FrustumLeft)].w = clipMatrix[3][3] + clipMatrix[0][3];

	_frustumPlanes[int(FrustumPlanes::FrustumBottom)].x = clipMatrix[3][0] + clipMatrix[1][0];
	_frustumPlanes[int(FrustumPlanes::FrustumBottom)].y = clipMatrix[3][1] + clipMatrix[1][1];
	_frustumPlanes[int(FrustumPlanes::FrustumBottom)].z = clipMatrix[3][2] + clipMatrix[1][2];
	_frustumPlanes[int(FrustumPlanes::FrustumBottom)].w = clipMatrix[3][3] + clipMatrix[1][3];

	_frustumPlanes[int(FrustumPlanes::FrustumTop)].x = clipMatrix[3][0] - clipMatrix[1][0];
	_frustumPlanes[int(FrustumPlanes::FrustumTop)].y = clipMatrix[3][1] - clipMatrix[1][1];
	_frustumPlanes[int(FrustumPlanes::FrustumTop)].z = clipMatrix[3][2] - clipMatrix[1][2];
	_frustumPlanes[int(FrustumPlanes::FrustumTop)].w = clipMatrix[3][3] - clipMatrix[1][3];

	_frustumPlanes[int(FrustumPlanes::FrustumFar)].x = clipMatrix[3][0] - clipMatrix[2][0];
	_frustumPlanes[int(FrustumPlanes::FrustumFar)].y = clipMatrix[3][1] - clipMatrix[2][1];
	_frustumPlanes[int(FrustumPlanes::FrustumFar)].z = clipMatrix[3][2] - clipMatrix[2][2];
	_frustumPlanes[int(FrustumPlanes::FrustumFar)].w = clipMatrix[3][3] - clipMatrix[2][3];

	_frustumPlanes[int(FrustumPlanes::FrustumNear)].x = clipMatrix[3][0] + clipMatrix[2][0];
	_frustumPlanes[int(FrustumPlanes::FrustumNear)].y = clipMatrix[3][1] + clipMatrix[2][1];
	_frustumPlanes[int(FrustumPlanes::FrustumNear)].z = clipMatrix[3][2] + clipMatrix[2][2];
	_frustumPlanes[int(FrustumPlanes::FrustumNear)].w = clipMatrix[3][3] + clipMatrix[2][3];

	for (int i = 0; i < int(FrustumPlanes::MaxPlanes); i++) {
		_frustumPlanes[i] = glm::normalize(_frustumPlanes[i]);
	}
}

}
