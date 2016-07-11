/**
 * @file
 */

#pragma once

#include "io/IEventObserver.h"
#include "core/GLM.h"
#include "core/Var.h"
#include "Ray.h"
#include <math.h>
#include <ctime>

namespace video {

enum class CameraType {
	FirstPerson,
	Free
};

enum class CameraRotationType {
	Target,
	Eye
};

enum class CameraMode {
	Perspective,
	Orthogonal
};

enum class FrustumPlanes {
	FrustumRight,
	FrustumLeft,
	FrustumTop,
	FrustumBottom,
	FrustumFar,
	FrustumNear,
	MaxPlanes
};

enum class FrustumResult {
	Outside,
	Inside,
	Intersect
};

class Camera {
private:
	constexpr static int DIRTY_ORIENTATION = 1 << 0;
	constexpr static int DIRTY_POSITON     = 1 << 1;
	constexpr static int DIRTY_FRUSTUM     = 1 << 2;
	constexpr static int DIRTY_TARGET      = 1 << 3;

	inline bool isDirty(int flag) const {
		if ((_dirty & flag) == 0) {
			return false;
		}
		return true;
	}

	CameraType _type;
	CameraMode _mode;
	CameraRotationType _rotationType = CameraRotationType::Eye;

	glm::vec3 _pos;
	glm::quat _quat;
	int _dirty = 0;

	glm::mat4 _viewMatrix;
	glm::mat4 _projectionMatrix;
	glm::mat4 _orientation;

	int _width = 0;
	int _height = 0;
	// rotation speed over time for all three axis
	glm::vec3 _omega;

	float _nearPlane = 0.1f;
	float _farPlane = 500.0f;
	float _aspectRatio = 1.0f;
	float _fieldOfView = 45.0f;

	float _distance = 100.0f;
	glm::vec3 _target;

	// Direction : Spherical coordinates to Cartesian coordinates conversion
	void updateFrustumPlanes();
	void updateViewMatrix();
	void updateOrientation();
	void updateProjectionMatrix();
	void updateTarget();
	glm::vec4 _frustumPlanes[int(FrustumPlanes::MaxPlanes)];
public:
	Camera(CameraType type = CameraType::FirstPerson, CameraMode mode = CameraMode::Perspective);
	~Camera();

	void init(int width, int height);
	int width() const;
	int height() const;

	CameraType type() const;
	void setType(CameraType type);

	CameraMode mode() const;
	void setMode(CameraMode mode);

	CameraRotationType rotationType() const;
	void setRotationType(CameraRotationType rotationType);

	float nearPlane() const;
	void setNearPlane(float nearPlane);

	float farPlane() const;
	void setFarPlane(float farPlane);

	glm::vec3 omega() const;
	void setOmega(const glm::vec3& omega);

	/**
	 * @return The rotation matrix of the direction the camera is facing to.
	 */
	glm::mat4 orientation() const;
	glm::quat quaternion() const;

	glm::vec3 forward() const;
	glm::vec3 right() const;
	glm::vec3 up() const;

	const glm::vec3& position() const;
	void setPosition(const glm::vec3& pos);
	void move(const glm::vec3& delta);

	glm::mat4 orthogonalMatrix() const;
	glm::mat4 perspectiveMatrix() const;
	const glm::mat4& viewMatrix() const;
	const glm::mat4& projectionMatrix() const;

	float fieldOfView() const;
	void setFieldOfView(float angles);

	float aspectRatio() const;
	void setAspectRatio(float aspect);

	/**
	 * @brief Rotation around the y-axis
	 */
	float yaw() const;
	void yaw(float radians);
	/**
	 * @brief Rotation around the z-axis
	 */
	float roll() const;
	void roll(float radians);
	/**
	 * @brief Rotation around the x-axis
	 */
	float pitch() const;
	void pitch(float radians);

	/**
	 * @brief Rotation around the y-axis relative to world up
	 */
	void turn(float radians);

	void rotate(float radians, const glm::vec3& axis);
	void rotate(const glm::quat& rotation);
	void rotate(int32_t relX, int32_t relY, float rotationSpeed = 0.1f);

	void lookAt(const glm::vec3& position);
	void lookAt(const glm::vec3& position, const glm::vec3& upDirection);

	void setTarget(const glm::vec3& target);
	void setTargetDistance(float distance);
	glm::vec3 target() const;
	float targetDistance() const;

	/**
	 * @param[in] pitch rotation in modelspace
	 * @param[in] yaw rotation in modelspace
	 * @param[in] roll rotation in modelspace
	 */
	void setAngles(float pitch, float yaw, float roll);

	void slerp(const glm::quat& quat, float factor);
	void slerp(float pitch, float yaw, float factor);
	void slerp(float pitch, float yaw, float roll, float factor);

	/**
	 * @brief Converts mouse coordinates into a ray
	 */
	Ray screenRay(const glm::vec2& screenPos) const;

	/**
	 * @brief Converts normalized screen coordinates [0.0-1.0] into world coordinates.
	 * @param[in] screenPos The normalized screen coordinates. The z component defines the length of the ray
	 * @param[in] projection The projection matrix
	 */
	glm::vec3 screenToWorld(const glm::vec3& screenPos) const;

	void update(long deltaFrame);

	const glm::vec4& frustumPlane(FrustumPlanes plane) const;
	FrustumResult testFrustum(const glm::vec3& position) const;
	FrustumResult testFrustum(const glm::vec3& mins, const glm::vec3& maxs) const;
};

inline void Camera::setType(CameraType type) {
	_type = type;
}

inline CameraType Camera::type() const {
	return _type;
}

inline void Camera::setMode(CameraMode mode) {
	_mode = mode;
}

inline CameraMode Camera::mode() const {
	return _mode;
}

inline CameraRotationType Camera::rotationType() const {
	return _rotationType;
}

inline void Camera::setRotationType(CameraRotationType rotationType) {
	_dirty |= DIRTY_TARGET;
	_rotationType = rotationType;
}

inline float Camera::pitch() const {
	return glm::pitch(_quat);
}

inline float Camera::roll() const {
	return glm::roll(_quat);
}

inline float Camera::yaw() const {
	return glm::yaw(_quat);
}

inline void Camera::pitch(float radians) {
	rotate(radians, glm::right);
}

inline void Camera::yaw(float radians) {
	rotate(radians, glm::up);
}

inline void Camera::roll(float radians) {
	rotate(radians, glm::backward);
}

inline void Camera::turn(float radians) {
	const glm::quat& quat = glm::angleAxis(radians, _quat * glm::up);
	rotate(quat);
}

inline void Camera::rotate(float radians, const glm::vec3& axis) {
	const glm::quat& quat = glm::angleAxis(radians, axis);
	rotate(quat);
}

inline void Camera::rotate(const glm::quat& rotation) {
	_quat = rotation * _quat;
}

inline void Camera::lookAt(const glm::vec3& position) {
	lookAt(position, up());
}

inline float Camera::nearPlane() const {
	return _nearPlane;
}

inline float Camera::farPlane() const {
	return _farPlane;
}

inline void Camera::setFarPlane(float farPlane) {
	_farPlane = farPlane;
}

inline void Camera::setNearPlane(float nearPlane) {
	_nearPlane = nearPlane;
}

inline glm::mat4 Camera::orientation() const {
	return _orientation;
}

inline glm::quat Camera::quaternion() const {
	return _quat;
}

inline glm::vec3 Camera::forward() const {
	return glm::conjugate(_quat) * glm::forward;
}

inline glm::vec3 Camera::right() const {
	return glm::conjugate(_quat) * glm::right;
}

inline glm::vec3 Camera::up() const {
	return glm::conjugate(_quat) * glm::up;
}

inline glm::mat4 Camera::orthogonalMatrix() const {
	const float w = width();
	const float h = height();
	return glm::ortho(0.0f, w, h, 0.0f, nearPlane(), farPlane());
}

inline void Camera::setAngles(float pitch, float yaw, float roll = 0.0f) {
	_quat = glm::quat(glm::vec3(pitch, yaw, roll));
	_dirty |= DIRTY_ORIENTATION;
}

inline void Camera::setPosition(const glm::vec3& pos) {
	if (glm::isEpsilonEqual(_pos, pos, 0.0001f)) {
		return;
	}
	_dirty |= DIRTY_POSITON;
	_pos = pos;
	if (_rotationType == CameraRotationType::Target) {
		lookAt(_target);
	}
}

inline const glm::mat4& Camera::viewMatrix() const {
	return _viewMatrix;
}

inline const glm::mat4& Camera::projectionMatrix() const {
	return _projectionMatrix;
}

inline const glm::vec4& Camera::frustumPlane(FrustumPlanes plane) const {
	return _frustumPlanes[int(plane)];
}

inline float Camera::fieldOfView() const {
	return _fieldOfView;
}

inline void Camera::setFieldOfView(float angles) {
	_fieldOfView = angles;
}

inline float Camera::aspectRatio() const {
	return _aspectRatio;
}

inline void Camera::setAspectRatio(float aspect) {
	_aspectRatio = aspect;
}

inline glm::mat4 Camera::perspectiveMatrix() const {
	return glm::perspective(glm::radians(_fieldOfView), _aspectRatio, nearPlane(), farPlane());
}

inline const glm::vec3& Camera::position() const {
	return _pos;
}

inline int Camera::width() const {
	return _width;
}

inline int Camera::height() const {
	return _height;
}

inline void Camera::setOmega(const glm::vec3& omega) {
	_omega = omega;
}

inline void Camera::setTarget(const glm::vec3& target) {
	if (glm::isEpsilonEqual(_target, target, 0.0001f)) {
		return;
	}
	_dirty |= DIRTY_TARGET;
	_target = target;
}

inline void Camera::setTargetDistance(float distance) {
	if (fabs(_distance - distance) < 0.0001f) {
		return;
	}
	_dirty |= DIRTY_TARGET;
	_distance = distance;
}

inline glm::vec3 Camera::target() const {
	return _target;
}

inline float Camera::targetDistance() const {
	return _distance;
}

}
