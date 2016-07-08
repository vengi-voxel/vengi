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

enum class CameraMode {
	FirstPerson,
	Free
};

enum class CameraType {
	Perspective,
	Orthogonal
};

class Camera {
private:
	constexpr static int DIRTY_ORIENTATION = 1 << 0;
	constexpr static int DIRTY_POSITON = 1 << 1;
	constexpr static int DIRTY_FRUSTUM = 1 << 2;

	inline bool isDirty(int flag) const {
		if ((_dirty & flag) == 0) {
			return false;
		}
		return true;
	}

	glm::vec3 _pos;
	glm::quat _quat;

	glm::mat4 _viewMatrix;
	glm::mat4 _projectionMatrix;
	glm::mat4 _orientation;

	int _width = 0;
	int _height = 0;
	// rotation speed over time for all three axis
	glm::vec3 _omega;
	glm::vec4 _frustumPlanes[int(FrustumPlanes::MaxPlanes)];
	float _nearPlane = 0.1f;
	float _farPlane = 500.0f;
	float _aspectRatio = 1.0f;
	float _fieldOfView = 45.0f;
	int _dirty = 0;
	bool _ortho;

	// Direction : Spherical coordinates to Cartesian coordinates conversion
	void updateFrustumPlanes();
	void updateViewMatrix();
	void updateOrientation();
	void updateProjectionMatrix();
public:
	Camera(bool ortho = false);
	~Camera();

	void init(int width, int height);

	void setOrtho(bool ortho);

	float nearPlane() const;

	float farPlane() const;

	void setFarPlane(float farPlane);

	void setNearPlane(float nearPlane);

	void setOmega(const glm::vec3& omega);

	/**
	 * @return The rotation matrix of the direction the camera is facing to.
	 */
	glm::mat4 orientation() const;

	glm::vec3 forward() const;

	glm::vec3 right() const;

	glm::vec3 up() const;

	void onMotion(int32_t relX, int32_t relY, float rotationSpeed = 0.1f);
	void onMovement(long dt, bool left, bool right, bool forward, bool backward, float speed = 0.01f);
	void update(long deltaFrame);

	FrustumResult testFrustum(const glm::vec3& position) const;
	FrustumResult testFrustum(const glm::vec3& mins, const glm::vec3& maxs) const;

	glm::mat4 orthoMatrix() const;
	glm::mat4 perspectiveMatrix() const;
	const glm::mat4& viewMatrix() const;
	const glm::mat4& projectionMatrix() const;

	void setFieldOfView(float angles);
	void setAspectRatio(float aspect);

	void lookAt(const glm::vec3& position);

	const glm::vec3& position() const;
	int width() const;
	int height() const;

	const glm::vec4& frustumPlane(FrustumPlanes plane) const;

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

	/**
	 * @param[in] pitch rotation in modelspace
	 * @param[in] yaw rotation in modelspace
	 * @param[in] roll rotation in modelspace
	 */
	void setAngles(float pitch, float yaw, float roll);
	void setPosition(const glm::vec3& pos);

	void slerp(float pitch, float yaw, float factor);

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
};

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
	glm::quat quat = glm::angleAxis(radians, _quat * glm::up);
	rotate(quat);
}

inline void Camera::rotate(float radians, const glm::vec3& axis) {
	glm::quat quat = glm::angleAxis(radians, axis);
	rotate(quat);
}

inline void Camera::rotate(const glm::quat& rotation) {
	_quat = rotation * _quat;
}

inline void Camera::setOrtho(bool ortho) {
	_ortho = ortho;
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

inline glm::vec3 Camera::forward() const {
	return glm::conjugate(_quat) * glm::forward;
}

inline glm::vec3 Camera::right() const {
	return glm::conjugate(_quat) * glm::right;
}

inline glm::vec3 Camera::up() const {
	return glm::conjugate(_quat) * glm::up;
}

inline glm::mat4 Camera::orthoMatrix() const {
	const float w = width();
	const float h = height();
	return glm::ortho(0.0f, w, h, 0.0f, nearPlane(), farPlane());
}

inline void Camera::setAngles(float pitch, float yaw, float roll = 0.0f) {
	_quat = glm::quat(glm::vec3(pitch, yaw, roll));
	_dirty |= DIRTY_ORIENTATION;
}

inline void Camera::setPosition(const glm::vec3& pos) {
	_dirty |= DIRTY_POSITON;
	_pos = pos;
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

inline void Camera::setFieldOfView(float angles) {
	_fieldOfView = angles;
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

}
