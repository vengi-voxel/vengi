/**
 * @file
 */

#pragma once

#include "math/Frustum.h"
#include "Types.h"
#include <glm/fwd.hpp>
#include <glm/vec2.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/quaternion.hpp>

namespace math {
template<typename TYPE>
class AABB;
class Ray;
}

namespace video {

enum class CameraType {
	FirstPerson,
	Free,
	UI,
	Max
};

enum class CameraRotationType {
	Target,
	Eye,
	Max
};

enum class CameraMode {
	Perspective,
	Orthogonal,
	Max
};

/**
 * @brief Camera class with frustum culling
 *
 * @ingroup Video
 * @par Coordinate spaces
 * There are several coordinate spaces in which the camera can be used:
 * @li object coordinates (the raw coordinates passed to glVertex, glVertexPointer etc)
 * @li eye coordinates (after the model-view matrix has been applied)
 * @li clip coordinates (after the projection matrix has been applied)
 * @li normalized device coordinates (after division by W)
 * @li window coordinates (after the viewport and depth-range transformations).
 */
class Camera {
protected:
	constexpr static uint32_t DIRTY_ORIENTATION = 1 << 0;
	constexpr static uint32_t DIRTY_POSITION    = 1 << 1;
	constexpr static uint32_t DIRTY_FRUSTUM     = 1 << 2;
	constexpr static uint32_t DIRTY_TARGET      = 1 << 3;
	constexpr static uint32_t DIRTY_PERSPECTIVE = 1 << 4;

	constexpr static uint32_t DIRTY_ALL = ~0u;
	constexpr static float ORTHO_BOXSIZE = 1.0f;

	inline bool isDirty(uint32_t flag) const {
		return (_dirty & flag) != 0u;
	}

	CameraType _type;
	CameraMode _mode;
	PolygonMode _polygonMode = PolygonMode::Solid;
	CameraRotationType _rotationType = CameraRotationType::Eye;

	glm::ivec2 _windowSize{0};
	float _orthoZoom = 1.0f;
	glm::vec3 _panOffset{0.0f};
	// the position of the camera in the world
	glm::vec3 _worldPos{0.0f};
	glm::quat _quat{0.0f, 0.0f, 0.0f, 0.0f};
	uint32_t _dirty = DIRTY_ALL;

	glm::mat4 _viewMatrix{1.0f};
	glm::mat4 _invViewMatrix{1.0f};
	glm::mat4 _projectionMatrix{1.0f};
	glm::mat4 _invProjectionMatrix{1.0f};
	glm::mat4 _viewProjectionMatrix{1.0f};
	glm::mat4 _orientation{1.0f};

	// rotation speed over time for all three axis
	glm::vec3 _omega{0.0f};

	float _nearPlane = 0.1f;
	float _farPlane = 500.0f;
	float _fieldOfView = 45.0f;

	glm::vec3 _target{0.0f};
	float _distance = 100.0f;
	bool _orthoAligned = false;
	bool _lerp = false;
	struct LerpTarget {
		CameraRotationType rotationType;
		glm::vec3 target;
		glm::vec3 worldPos;
		glm::vec3 panOffset;
		glm::quat quat;
		glm::vec3 fromWorldPos;
		glm::vec3 fromPanOffset;
		glm::quat fromQuat;
		glm::vec3 fromTarget;
		double seconds;
		float distance;
		float fromDistance;
		float fieldOfView;
		float fromFieldOfView;
		float orthoZoom;
		float fromOrthoZoom;
	};
	LerpTarget _lerpTarget;

	void updateFrustumPlanes();
	void updateFrustumVertices();
	void updateViewMatrix();
	void updateOrientation();
	void updateProjectionMatrix();
	void updateTarget();
	void updateLerp(double deltaFrameSeconds);

	math::Frustum _frustum;
public:
	Camera(CameraType type = CameraType::FirstPerson, CameraMode mode = CameraMode::Perspective);

	const glm::ivec2& size() const;
	void setSize(const glm::ivec2& windowSize);
	int frameBufferHeight() const;

	inline bool dirty() const {
		return _dirty != 0u;
	}

	glm::vec3 eye() const;

	CameraType type() const;
	void setType(CameraType type);

	void resetZoom();
	void zoom(float value);
	float aspect() const;

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
	void setOrientation(const glm::quat& quat);

	bool isOrthoAligned() const;

	void lerp(const Camera& target);

	glm::vec3 forward() const;
	glm::vec3 right() const;
	glm::vec3 up() const;

	glm::vec3 direction() const;

	/**
	 * @sa eye()
	 * @note Does not include panning offset
	 */
	const glm::vec3 &worldPosition() const;
	const glm::vec3 &panOffset() const;
	void setWorldPosition(const glm::vec3 &worldPos);
	bool move(const glm::vec3& delta);

	glm::mat4 orthogonalMatrix(float nplane, float fplane) const;
	glm::mat4 perspectiveMatrix(float nplane, float fplane) const;
	const glm::mat4& inverseViewMatrix() const;
	/**
	 * from world space to camera space
	 */
	const glm::mat4& viewMatrix() const;
	/**
	 * camera space to screen space
	 */
	const glm::mat4& projectionMatrix() const;
	const glm::mat4& inverseProjectionMatrix() const;
	const glm::mat4& viewProjectionMatrix() const;

	/**
	 * @return the field of view in degree
	 */
	float fieldOfView() const;
	/**
	 * @param angles in degree
	 */
	void setFieldOfView(float angles);

	/**
	 * @brief Rotation around the y-axis
	 */
	float yaw() const;
	float horizontalYaw() const;
	void setYaw(float radians);
	/**
	 * @brief Rotation around the z-axis
	 */
	float roll() const;
	void setRoll(float radians);
	/**
	 * @brief Rotation around the x-axis
	 */
	float pitch() const;
	void setPitch(float radians);

	/**
	 * @brief Rotation around the y-axis relative to world up
	 */
	void turn(float radians);

	void pan(int x, int y);

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
	 * @param[in] pitch rotation in modelspace in radians
	 * @param[in] yaw rotation in modelspace in radians
	 * @param[in] roll rotation in modelspace in radians
	 */
	void setAngles(float pitch, float yaw, float roll);

	void slerp(const glm::quat& quat, float factor);
	void slerp(const glm::vec3& radians, float factor);

	/**
	 * @param[in] pixelPos screen pixel position
	 * @note Basically just a wrapper for @c screenRay() but for mouse coordinates
	 * @return Ray instance with origin and direction
	 */
	math::Ray mouseRay(const glm::ivec2& pixelPos) const;

	glm::ivec2 worldToScreen(const glm::vec3& worldPos) const;
	glm::ivec2 worldToScreen(const glm::mat4& matrix, const glm::vec3& worldPos) const;

	void update(double deltaFrameSeconds);

	/**
	 * @brief Split the current frustum by @c bufSize steps
	 * @param[out] sliceBuf the target buffer for the near/far plane combos
	 * @param[in] splits The amount of splits. The bufSize must be at least @code splits * 2 @endcode
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

	bool operator==(const Camera& other) const;
	inline bool operator!=(const Camera& other) const {
		return !(*this == other);
	}
};

inline glm::vec3 Camera::eye() const {
	return _invViewMatrix[3];
}

inline const glm::ivec2& Camera::size() const {
	return _windowSize;
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
	if (_rotationType == rotationType) {
		return;
	}
	_dirty |= DIRTY_TARGET;
	_rotationType = rotationType;
}

inline PolygonMode Camera::polygonMode() const {
	return _polygonMode;
}

inline void Camera::setPolygonMode(PolygonMode polygonMode) {
	_polygonMode = polygonMode;
}

inline float Camera::nearPlane() const {
	return _nearPlane;
}

inline float Camera::farPlane() const {
	return _farPlane;
}

inline float Camera::aspect() const {
	if (_windowSize.y == 0) {
		return 1.0f;
	}
	if (_mode == CameraMode::Orthogonal) {
		return (float)_windowSize.x / (float)_windowSize.y;
	}
	return (float)_windowSize.y / (float)_windowSize.x;
}

inline const glm::mat4& Camera::orientation() const {
	return _orientation;
}

inline const glm::quat& Camera::quaternion() const {
	return _quat;
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

inline const glm::vec3& Camera::worldPosition() const {
	return _worldPos;
}

inline const glm::vec3& Camera::panOffset() const {
	return _panOffset;
}

inline void Camera::setTargetDistance(float distance) {
	if (glm::abs(_distance - distance) < 0.0001f) {
		return;
	}
	_dirty |= DIRTY_TARGET;
	_distance = distance;
}

inline glm::vec3 Camera::omega() const {
	return _omega;
}

inline glm::vec3 Camera::target() const {
	return _target;
}

inline float Camera::targetDistance() const {
	return _distance;
}

inline Camera uiCamera(const glm::ivec2& windowSize) {
	Camera camera(CameraType::UI, video::CameraMode::Orthogonal);
	camera.setSize(windowSize);
	camera.setNearPlane(-1.0f);
	camera.setFarPlane(1.0f);
	camera.update(0.0);
	return camera;
}

}
