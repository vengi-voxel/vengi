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
	float _pitch;
	float _yaw;
	glm::vec3 _direction;
	core::VarPtr _maxpitch;
	glm::vec4 _frustumPlanes[int(FrustumPlanes::MaxPlanes)];
	float _farPlane = 500.0f;

	// Direction : Spherical coordinates to Cartesian coordinates conversion
	void updateDirection();
	void updateFrustumPlanes();
	void updateViewMatrix();
public:
	Camera();
	~Camera();
	void init(int width, int height);

	inline float nearPlane() const {
		return 0.1f;
	}

	inline float farPlane() const {
		return _farPlane;
	}

	inline void setFarPlane(float farPlane) {
		_farPlane = farPlane;
	}

	void onMotion(int32_t x, int32_t y, int32_t relX, int32_t relY, float rotationSpeed = 0.01f);
	void onMovement(int32_t forward, int32_t sideward);
	void updatePosition(long dt, bool left, bool right, bool forward, bool backward, float speed = 0.01f);
	void update();

	FrustumResult testFrustum(const glm::vec3& position) const;
	FrustumResult testFrustum(const glm::vec3& mins, const glm::vec3& maxs) const;

	void perspective(float fieldOfViewY, float aspectRatio);

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

inline void Camera::perspective(float fieldOfViewY, float aspectRatio) {
	_projectionMatrix = glm::perspective(fieldOfViewY, aspectRatio, nearPlane(), farPlane());
}

inline void Camera::update() {
	updateDirection();
	updateViewMatrix();
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
