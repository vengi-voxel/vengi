/**
 * @file
 */

#pragma once

#include "Types.h"
#include "math/Frustum.h"
#include "math/Quatf.h"
#include "math/Mat4f.h"
#include "math/Vec4f.h"
#include "math/Vec3f.h"
#include "math/Vec2i.h"
#include "math/Vec2f.h"
#include "math/Scalar.h"

namespace math {
template<typename TYPE>
class AABB;
class Ray;
} // namespace math

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
	Isometric,
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

	math::Vec2i _windowSize{0};
	float _orthoZoom = 1.0f;
	float _orthoDepth = 10000.0f; // total depth span
	// the position of the camera in the world
	math::Vec3f _worldPos{0.0f};
	math::Quatf _quat = math::Quatf::identity();
	uint32_t _dirty = DIRTY_ALL;

	math::Mat4f _viewMatrix{1.0f};
	math::Mat4f _invViewMatrix{1.0f};
	math::Mat4f _projectionMatrix{1.0f};
	math::Mat4f _invProjectionMatrix{1.0f};
	math::Mat4f _viewProjectionMatrix{1.0f};
	math::Mat4f _orientation{1.0f};

	// rotation speed over time for all three axis
	math::Vec3f _omega{0.0f};

	float _nearPlane = 0.1f;
	float _farPlane = 500.0f;
	// y and in degrees
	float _fieldOfView = 45.0f;

	math::Vec3f _target{0.0f};
	float _distance = 100.0f;
	bool _orthoAligned = false;
	bool _lerp = false;
	struct LerpTarget {
		CameraRotationType rotationType;
		math::Vec3f target;
		math::Vec3f worldPos;
		math::Quatf quat = math::Quatf::identity();
		math::Vec3f fromWorldPos;
		math::Quatf fromQuat = math::Quatf::identity();
		math::Vec3f fromTarget;
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
	static constexpr float IsometricPitch = 0.6154797f;
	static constexpr float PIXELS_PER_UNIT = 25.0f;

	Camera(CameraType type = CameraType::FirstPerson, CameraMode mode = CameraMode::Perspective);

	const math::Vec2i &size() const;
	void setSize(const math::Vec2i &windowSize);
	int frameBufferHeight() const;

	inline bool dirty() const {
		return _dirty != 0u;
	}

	math::Vec3f eye() const;

	CameraType type() const;
	void setType(CameraType type);

	void resetZoom();
	void zoom(float value);
	float aspect() const;

	CameraMode mode() const;
	void setMode(CameraMode mode);
	bool isOrthographic() const;
	bool isIsometric() const;

	CameraRotationType rotationType() const;
	void setRotationType(CameraRotationType rotationType);

	PolygonMode polygonMode() const;
	void setPolygonMode(PolygonMode polygonMode);

	void setOrthoDepth(float depth);

	float nearPlane() const;
	void setNearPlane(float nearPlane);

	float farPlane() const;
	void setFarPlane(float farPlane);

	math::Vec3f omega() const;
	void setOmega(const math::Vec3f &omega);

	/**
	 * @return The rotation matrix of the direction the camera is facing to.
	 */
	const math::Mat4f &orientation() const;
	const math::Quatf &quaternion() const;
	void setOrientation(const math::Quatf &quat);

	float orthoZoom() const;
	void setOrthoZoom(float zoom);
	bool isOrthoAligned() const;

	void lerp(const Camera &target);

	math::Vec3f forward() const;
	math::Vec3f right() const;
	math::Vec3f up() const;

	math::Vec3f direction() const;

	/**
	 * @sa eye()
	 * @note Does not include panning offset
	 */
	const math::Vec3f &worldPosition() const;
	void setWorldPosition(const math::Vec3f &worldPos);
	bool move(const math::Vec3f &delta);

	math::Mat4f orthogonalMatrix(float nplane, float fplane) const;
	math::Mat4f perspectiveMatrix(float nplane, float fplane) const;
	const math::Mat4f &inverseViewMatrix() const;
	/**
	 * from world space to camera space
	 */
	const math::Mat4f &viewMatrix() const;
	/**
	 * camera space to screen space
	 */
	const math::Mat4f &projectionMatrix() const;
	const math::Mat4f &inverseProjectionMatrix() const;
	const math::Mat4f &viewProjectionMatrix() const;

	/**
	 * @return the vertical field of view in degree
	 */
	float fieldOfView() const;
	/**
	 * @param angles in degree (vertical field of view)
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

	/** @brief World-space scale of one pixel on the navigation target plane. */
	float worldUnitsPerPixel() const;
	void pan(int screenDeltaX, int screenDeltaY);

	void rotate(float radians, const math::Vec3f &axis);
	void rotate(const math::Quatf &rotation);
	void rotate(const math::Vec3f &radians);

	bool lookAt(const math::Vec3f &position);
	bool lookAt(const math::Vec3f &position, const math::Vec3f &upDirection);

	/**
	 * @brief Calculates the billboard vectors right and up relative to the camera.
	 */
	void billboard(math::Vec3f *right, math::Vec3f *up) const;

	void setTarget(const math::Vec3f &target);
	void setTargetDistance(float distance);
	math::Vec3f target() const;
	float targetDistance() const;

	/**
	 * @param[in] pitch rotation in modelspace in radians
	 * @param[in] yaw rotation in modelspace in radians
	 * @param[in] roll rotation in modelspace in radians
	 */
	void setAngles(float pitch, float yaw, float roll);

	void slerp(const math::Quatf &quat, float factor);
	void slerp(const math::Vec3f &radians, float factor);

	/**
	 * @param[in] pixelPos screen pixel position
	 * @note Basically just a wrapper for @c screenRay() but for mouse coordinates
	 * @return Ray instance with origin and direction
	 */
	math::Ray mouseRay(const math::Vec2i &pixelPos) const;
	/**
	 * @brief Map a top-left logical point in a viewport to a world ray.
	 * Viewport origin/size and point must use the same units. Far edges are excluded.
	 * Accounts for camera resolution (including high DPI). Leaves ray unchanged on failure.
	 */
	bool mouseRay(const math::Vec4f &view, const math::Vec2f &point, math::Ray &ray) const;

	math::Vec2i worldToScreen(const math::Vec3f &worldPos) const;
	/** Clip a world segment to the view volume before perspective division. */
	bool worldSegmentToScreen(const math::Vec3f &from, const math::Vec3f &to, math::Vec2i &screenFrom,
							  math::Vec2i &screenTo) const;
	math::Vec2i worldToScreen(const math::Mat4f &matrix, const math::Vec3f &worldPos) const;

	void update(double deltaFrameSeconds);

	/**
	 * @brief Split the current frustum by @c bufSize steps
	 * @param[out] sliceBuf the target buffer for the near/far plane combos
	 * @param[in] splits The amount of splits. The bufSize must be at least @code splits * 2 @endcode
	 */
	void sliceFrustum(float *sliceBuf, int bufSize, int splits, float sliceWeight = 0.75f) const;
	/**
	 * @brief Calculates the 8 vertices for a split frustum
	 */
	void splitFrustum(float nearPlane, float farPlane, math::Vec3f out[math::FRUSTUM_VERTICES_MAX]) const;
	void frustumCorners(math::Vec3f out[math::FRUSTUM_VERTICES_MAX], uint32_t indices[24]) const;
	const math::Frustum &frustum() const;
	bool isVisible(const math::Vec3f &position) const;
	bool isVisible(const math::AABB<float> &aabb) const;
	bool isVisible(const math::Vec3f &mins, const math::Vec3f &maxs) const;
	math::AABB<float> aabb() const;
	math::Vec4f sphereBoundingBox() const;

	bool operator==(const Camera &other) const;
	inline bool operator!=(const Camera &other) const {
		return !(*this == other);
	}
};

inline math::Vec3f Camera::eye() const {
	return _invViewMatrix[3];
}

inline const math::Vec2i &Camera::size() const {
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

inline bool Camera::isOrthographic() const {
	return _mode == CameraMode::Orthogonal || _mode == CameraMode::Isometric;
}

inline bool Camera::isIsometric() const {
	return _mode == CameraMode::Isometric;
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
	if (isOrthographic()) {
		return -_orthoDepth * 0.5f;
	}
	return _nearPlane;
}

inline float Camera::farPlane() const {
	if (isOrthographic()) {
		return _orthoDepth * 0.5f;
	}
	return _farPlane;
}

inline float Camera::aspect() const {
	if (_windowSize.y == 0) {
		return 1.0f;
	}
	if (isOrthographic()) {
		return (float)_windowSize.x / (float)_windowSize.y;
	}
	return (float)_windowSize.y / (float)_windowSize.x;
}

inline const math::Mat4f &Camera::orientation() const {
	return _orientation;
}

inline const math::Quatf &Camera::quaternion() const {
	return _quat;
}

inline const math::Mat4f &Camera::inverseViewMatrix() const {
	return _invViewMatrix;
}

inline const math::Mat4f &Camera::viewMatrix() const {
	return _viewMatrix;
}

inline const math::Mat4f &Camera::projectionMatrix() const {
	return _projectionMatrix;
}

inline const math::Mat4f &Camera::inverseProjectionMatrix() const {
	return _invProjectionMatrix;
}

inline const math::Mat4f &Camera::viewProjectionMatrix() const {
	return _viewProjectionMatrix;
}

inline const math::Frustum &Camera::frustum() const {
	return _frustum;
}

inline bool Camera::isVisible(const math::Vec3f &position) const {
	return frustum().isVisible(position);
}

inline bool Camera::isVisible(const math::Vec3f &mins, const math::Vec3f &maxs) const {
	return frustum().isVisible(mins, maxs);
}

inline void Camera::frustumCorners(math::Vec3f out[math::FRUSTUM_VERTICES_MAX], uint32_t indices[24]) const {
	frustum().corners(out, indices);
}

inline float Camera::fieldOfView() const {
	return _fieldOfView;
}

inline void Camera::setFieldOfView(float angles) {
	_dirty |= DIRTY_PERSPECTIVE;
	_fieldOfView = angles;
}

inline const math::Vec3f &Camera::worldPosition() const {
	return _worldPos;
}

inline void Camera::setTargetDistance(float distance) {
	if (math::abs(_distance - distance) < 0.0001f) {
		return;
	}
	_dirty |= DIRTY_TARGET;
	_distance = distance;
}

inline math::Vec3f Camera::omega() const {
	return _omega;
}

inline math::Vec3f Camera::target() const {
	return _target;
}

inline float Camera::targetDistance() const {
	return _distance;
}

inline Camera uiCamera(const math::Vec2i &windowSize) {
	Camera camera(CameraType::UI, video::CameraMode::Orthogonal);
	camera.setSize(windowSize);
	camera.setNearPlane(-1.0f);
	camera.setFarPlane(1.0f);
	camera.update(0.0);
	return camera;
}

inline float Camera::orthoZoom() const {
	return _orthoZoom;
}

inline void Camera::setOrthoZoom(float zoom) {
	const float value = math::max(zoom, 0.0001f);
	if (math::epsilonEqual(_orthoZoom, value, 0.0001f)) {
		return;
	}
	_orthoZoom = value;
	_dirty |= DIRTY_PERSPECTIVE;
}

} // namespace video
