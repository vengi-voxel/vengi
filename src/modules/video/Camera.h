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

class Camera {
private:
	glm::vec3 _pos;
	glm::mat4 _viewMatrix;
	glm::mat4 _projectionMatrix;
	int _width;
	int _height;
	// vertical angle
	float _pitch;
	// horizontal angle
	float _yaw;
	glm::vec3 _direction;
	core::VarPtr _maxpitch;
	glm::vec4 _frustumPlanes[int(FrustumPlanes::MaxPlanes)];
	float _nearPlane = 0.1f;
	float _farPlane = 500.0f;
	float _aspectRatio = 1.0f;
	float _fieldOfView = 45.0f;
	bool _ortho;

	// Direction : Spherical coordinates to Cartesian coordinates conversion
	void updateDirection();
	void updateFrustumPlanes();
	void updateViewMatrix();
	void updateProjectionMatrix();

	void normalizeAngles();
public:
	Camera(bool ortho = false);
	~Camera();

	void init(int width, int height);

	void setOrtho(bool ortho);

	float nearPlane() const;

	float farPlane() const;

	void setFarPlane(float farPlane);

	void setNearPlane(float nearPlane);

	/**
	 * @return The rotation matrix of the direction the camera is facing to.
	 */
	glm::mat4 orientation() const;

	glm::vec3 forward() const;

	glm::vec3 right() const;

	glm::vec3 up() const;

	void onMotion(int32_t x, int32_t y, int32_t relX, int32_t relY, float rotationSpeed = 0.01f);
	void onMovement(int32_t forward, int32_t sideward);
	void updatePosition(long dt, bool left, bool right, bool forward, bool backward, float speed = 0.01f);
	void update();

	FrustumResult testFrustum(const glm::vec3& position) const;
	FrustumResult testFrustum(const glm::vec3& mins, const glm::vec3& maxs) const;

	glm::mat4 orthoMatrix() const;
	glm::mat4 perspectiveMatrix() const;

	void setFieldOfView(float angles);
	void setAspectRatio(float aspect);

	void lookAt(const glm::vec3& position);

	const glm::vec3& position() const;
	int width() const;
	int height() const;
	const glm::mat4& viewMatrix() const;
	const glm::mat4& projectionMatrix() const;
	const glm::vec4& frustumPlane(FrustumPlanes plane) const;
	/**
	 * @brief Return the horizontal angle of the camera view direction
	 */
	float yaw() const;
	/**
	 * @brief Return the vertical angle of the camera view direction
	 */
	float pitch() const;

	void setAngles(float pitch, float yaw);
	void setPosition(const glm::vec3& pos);

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

inline float Camera::yaw() const {
	return _yaw;
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
	const glm::mat4& orientation = glm::rotate(glm::rotate(glm::mat4(), glm::radians(_pitch), glm::vec3(1, 0, 0)), glm::radians(_yaw), glm::vec3(0, 1, 0));
	return orientation;
}

inline glm::vec3 Camera::forward() const {
	return (glm::inverse(orientation()) * glm::vec4(0, 0, -1, 1)).xyz();
}

inline glm::vec3 Camera::right() const {
	return (glm::inverse(orientation()) * glm::vec4(1, 0, 0, 1)).xyz();
}

inline glm::vec3 Camera::up() const {
	return (glm::inverse(orientation()) * glm::vec4(0, 1, 0, 1)).xyz();
}

inline glm::mat4 Camera::orthoMatrix() const {
	const float w = width();
	const float h = height();
	return glm::ortho(0.0f, w, h, 0.0f, nearPlane(), farPlane());
}

inline float Camera::pitch() const {
	return _pitch;
}

inline void Camera::setAngles(float pitch, float yaw) {
	_pitch = pitch;
	_yaw = yaw;
}

inline void Camera::setPosition(const glm::vec3& pos) {
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

inline void Camera::updateProjectionMatrix() {
	if (_ortho) {
		_projectionMatrix = orthoMatrix();
	} else {
		_projectionMatrix = perspectiveMatrix();
	}
}

inline void Camera::update() {
	updateDirection();
	updateViewMatrix();
	updateProjectionMatrix();
	updateFrustumPlanes();
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

inline void Camera::updateViewMatrix() {
	_viewMatrix = glm::lookAt(_pos, _pos + glm::normalize(_direction), glm::vec3(0.0, 1.0, 0.0));
}

}
