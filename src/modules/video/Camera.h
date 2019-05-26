/**
 * @file
 */

#pragma once

#include "core/GLM.h"
#include "core/Var.h"
#include "math/AABB.h"
#include "math/Frustum.h"
#include "Types.h"
#include "Ray.h"
#include <math.h>
#include <time.h>

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

/**
 * @brief Camera class with frustum culling
 *
 * @ingroup Video
 * @par Coordinate spaces
 * @li object coordinates (the raw coordinates passed to glVertex, glVertexPointer etc)
 * @li eye coordinates (after the model-view matrix has been applied)
 * @li clip coordinates (after the projection matrix has been applied)
 * @li normalized device coordinates (after division by W)
 * @li window coordinates (after the viewport and depth-range transformations).
 */
class Camera {
protected:
	constexpr static uint32_t DIRTY_ORIENTATION = 1 << 0;
	constexpr static uint32_t DIRTY_POSITON     = 1 << 1;
	constexpr static uint32_t DIRTY_FRUSTUM     = 1 << 2;
	constexpr static uint32_t DIRTY_TARGET      = 1 << 3;
	constexpr static uint32_t DIRTY_PERSPECTIVE = 1 << 4;

	constexpr static uint32_t DIRTY_ALL = ~0u;

	inline bool isDirty(uint32_t flag) const {
		return (_dirty & flag) != 0u;
	}

	CameraType _type;
	CameraMode _mode;
	PolygonMode _polygonMode = PolygonMode::Solid;
	CameraRotationType _rotationType = CameraRotationType::Eye;

	glm::ivec2 _dimension {0};
	// the position that is used for ortho projection matrices
	glm::ivec2 _position {0};
	// the position of the camera in the world
	glm::vec3 _pos {0.0f};
	glm::quat _quat = glm::quat(1, 0, 0, 0);
	uint32_t _dirty = DIRTY_ALL;

	glm::mat4 _viewMatrix = glm::mat4(1.0f);
	glm::mat4 _invViewMatrix = glm::mat4(1.0f);
	glm::mat4 _projectionMatrix = glm::mat4(1.0f);
	glm::mat4 _invProjectionMatrix = glm::mat4(1.0f);
	glm::mat4 _viewProjectionMatrix = glm::mat4(1.0f);
	glm::mat4 _orientation = glm::mat4(1.0f);

	// rotation speed over time for all three axis
	glm::vec3 _omega {0.0f};

	float _nearPlane = 0.1f;
	float _farPlane = 500.0f;
	float _aspectRatio = 1.0f;
	float _fieldOfView = 45.0f;

	glm::vec3 _target {0.0f};
	float _distance = 100.0f;

	void updateFrustumPlanes();
	void updateFrustumVertices();
	void updateViewMatrix();
	void updateOrientation();
	void updateProjectionMatrix();
	void updateTarget();

	math::Frustum _frustum;
public:
	Camera(CameraType type = CameraType::FirstPerson, CameraMode mode = CameraMode::Perspective);

	void init(const glm::ivec2& position, const glm::ivec2& dimension);
	const glm::ivec2& dimension() const;
	int width() const;
	int height() const;

	/**
	 * @note Not the world position of the camera - but for controlling the viewport
	 */
	int x() const;
	/**
	 * @note Not the world position of the camera - but for controlling the viewport
	 */
	int y() const;

	CameraType type() const;
	void setType(CameraType type);

	CameraMode mode() const;
	void setMode(CameraMode mode);

	CameraRotationType rotationType() const;
	void setRotationType(CameraRotationType rotationType);

	PolygonMode polygonMode() const;
	void setPolygonMode(PolygonMode polygonMode);

	float nearPlane() const;
	void setNearPlane(float nearPlane);

	float farPlane() const;
	void setFarPlane(float farPlane);

	glm::vec3 omega() const;
	void setOmega(const glm::vec3& omega);

	/**
	 * @return The rotation matrix of the direction the camera is facing to.
	 */
	const glm::mat4& orientation() const;
	const glm::quat& quaternion() const;
	void setQuaternion(const glm::quat& quat);

	glm::vec3 forward() const;
	glm::vec3 right() const;
	glm::vec3 up() const;

	glm::vec3 direction() const;

	const glm::vec3& position() const;
	void setPosition(const glm::vec3& pos);
	void move(const glm::vec3& delta);

	glm::mat4 orthogonalMatrix() const;
	glm::mat4 perspectiveMatrix() const;
	const glm::mat4& inverseViewMatrix() const;
	const glm::mat4& viewMatrix() const;
	const glm::mat4& projectionMatrix() const;
	const glm::mat4& inverseProjectionMatrix() const;
	const glm::mat4& viewProjectionMatrix() const;

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
	void rotate(const glm::vec3& radians);

	bool lookAt(const glm::vec3& position);
	bool lookAt(const glm::vec3& position, const glm::vec3& upDirection);

	/**
	 * @brief Calculates the billboard vectors right and up relative to the camera.
	 */
	void billboard(glm::vec3 *right, glm::vec3 *up) const;

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
	void slerp(const glm::vec3& radians, float factor);

	/**
	 * @brief Converts normalized mouse coordinates into a ray
	 * project screen position [0.0-1.0] to [-1.0,1.0] and flip y axis
	 * to bring them into homogeneous clip coordinates
	 * @param[in] screenPos normalized screen position [0.0-1.0]
	 * @return Ray instance with origin and direction
	 */
	Ray screenRay(const glm::vec2& screenPos) const;
	/**
	 * @param[in] screenPos screen pixel position
	 * @note Basically just a wrapper for @c screenRay() but for mouse coordinates
	 * @return Ray instance with origin and direction
	 */
	Ray mouseRay(const glm::ivec2& screenPos) const;

	/**
	 * @brief Converts normalized screen coordinates [0.0-1.0] into world coordinates.
	 * @param[in] screenPos The normalized screen coordinates. The z component defines the length of the ray
	 */
	glm::vec3 screenToWorld(const glm::vec3& screenPos) const;

	void update(uint64_t deltaFrame);

	/**
	 * @brief Split the current frustum by @c bufSize steps
	 * @param[out] sliceBuf the target buffer for the near/far plane combos
	 * @param[in] splits The amount of splits. The bufSize must be at least @code splits * 2 @encode
	 */
	void sliceFrustum(float* sliceBuf, int bufSize, int splits, float sliceWeight = 0.75f) const;
	/**
	 * @brief Calculates the 8 vertices for a split frustum
	 */
	void splitFrustum(float nearPlane, float farPlane, glm::vec3 out[math::FRUSTUM_VERTICES_MAX]) const;
	void frustumCorners(glm::vec3 out[math::FRUSTUM_VERTICES_MAX], uint32_t indices[24]) const;
	const math::Frustum& frustum() const;
	bool isVisible(const glm::vec3& position) const;
	bool isVisible(const math::AABB<float>& aabb) const;
	bool isVisible(const glm::vec3& mins, const glm::vec3& maxs) const;
	math::AABB<float> aabb() const;
	glm::vec4 sphereBoundingBox() const;
	glm::vec4 splitFrustumSphereBoundingBox(float near, float far) const;
};

inline const glm::ivec2& Camera::dimension() const {
	return _dimension;
}

inline int Camera::width() const {
	return _dimension.x;
}

inline int Camera::height() const {
	return _dimension.y;
}

inline int Camera::x() const {
	return _position.x;
}

inline int Camera::y() const {
	return _position.y;
}

inline void Camera::setType(CameraType type) {
	_type = type;
}

inline CameraType Camera::type() const {
	return _type;
}

inline void Camera::setMode(CameraMode mode) {
	if (mode == _mode) {
		return;
	}
	_mode = mode;
	_dirty = DIRTY_ALL;
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

inline PolygonMode Camera::polygonMode() const {
	return _polygonMode;
}

inline void Camera::setPolygonMode(PolygonMode polygonMode) {
	_polygonMode = polygonMode;
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
	if (fabs(radians) < 0.00001f) {
		return;
	}
	const glm::quat& quat = glm::angleAxis(radians, _quat * glm::up);
	rotate(quat);
}

inline void Camera::rotate(float radians, const glm::vec3& axis) {
	if (fabs(radians) < 0.00001f) {
		return;
	}
	const glm::quat& quat = glm::angleAxis(radians, axis);
	rotate(quat);
}

inline void Camera::rotate(const glm::quat& rotation) {
	core_assert(!glm::any(glm::isnan(rotation)));
	core_assert(!glm::any(glm::isinf(rotation)));
	_quat = glm::normalize(rotation * _quat);
	_dirty |= DIRTY_ORIENTATION;
}

inline bool Camera::lookAt(const glm::vec3& position) {
	return lookAt(position, glm::up);
}

inline float Camera::nearPlane() const {
	return _nearPlane;
}

inline float Camera::farPlane() const {
	return _farPlane;
}

inline const glm::mat4& Camera::orientation() const {
	return _orientation;
}

inline const glm::quat& Camera::quaternion() const {
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

inline void Camera::setAngles(float pitch, float yaw, float roll = 0.0f) {
	_quat = glm::quat(glm::vec3(pitch, yaw, roll));
	core_assert(!glm::any(glm::isnan(_quat)));
	core_assert(!glm::any(glm::isinf(_quat)));
	_dirty |= DIRTY_ORIENTATION;
}

inline void Camera::setQuaternion(const glm::quat& quat) {
	_quat = quat;
	core_assert(!glm::any(glm::isnan(_quat)));
	core_assert(!glm::any(glm::isinf(_quat)));
	_dirty |= DIRTY_ORIENTATION;
}

inline void Camera::setPosition(const glm::vec3& pos) {
	if (glm::all(glm::epsilonEqual(_pos, pos, 0.0001f))) {
		return;
	}
	_dirty |= DIRTY_POSITON;
	_pos = pos;
	if (_rotationType == CameraRotationType::Target) {
		lookAt(_target);
	}
}

inline const glm::mat4& Camera::inverseViewMatrix() const {
	return _invViewMatrix;
}

inline const glm::mat4& Camera::viewMatrix() const {
	return _viewMatrix;
}

inline const glm::mat4& Camera::projectionMatrix() const {
	return _projectionMatrix;
}

inline const glm::mat4& Camera::inverseProjectionMatrix() const {
	return _invProjectionMatrix;
}

inline const glm::mat4& Camera::viewProjectionMatrix() const {
	return _viewProjectionMatrix;
}

inline const math::Frustum& Camera::frustum() const {
	return _frustum;
}

inline bool Camera::isVisible(const glm::vec3& position) const {
	return frustum().isVisible(position);
}

inline bool Camera::isVisible(const math::AABB<float>& aabb) const {
	return isVisible(aabb.getLowerCorner(), aabb.getUpperCorner());
}

inline bool Camera::isVisible(const glm::vec3& mins, const glm::vec3& maxs) const {
	return frustum().isVisible(mins, maxs);
}

inline void Camera::frustumCorners(glm::vec3 out[math::FRUSTUM_VERTICES_MAX], uint32_t indices[24]) const {
	frustum().corners(out, indices);
}

inline float Camera::fieldOfView() const {
	return _fieldOfView;
}

inline void Camera::setFieldOfView(float angles) {
	_dirty |= DIRTY_PERSPECTIVE;
	_fieldOfView = angles;
}

inline float Camera::aspectRatio() const {
	return _aspectRatio;
}

inline void Camera::setAspectRatio(float aspect) {
	_dirty |= DIRTY_PERSPECTIVE;
	_aspectRatio = aspect;
}

inline glm::vec3 Camera::direction() const {
	return glm::vec3(glm::column(inverseViewMatrix(), 2));
}

inline const glm::vec3& Camera::position() const {
	return _pos;
}

inline void Camera::setOmega(const glm::vec3& omega) {
	core_assert(!glm::any(glm::isnan(omega)));
	core_assert(!glm::any(glm::isinf(omega)));
	_omega = omega;
}

inline void Camera::setTarget(const glm::vec3& target) {
	core_assert(!glm::any(glm::isnan(target)));
	core_assert(!glm::any(glm::isinf(target)));
	if (glm::all(glm::epsilonEqual(_target, target, 0.0001f))) {
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
