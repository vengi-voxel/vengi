/**
 * @file
 */

#pragma once

#include "Plane.h"
#include <stddef.h>
#include <stdint.h>
#include "math/Mat4f.h"
#include "math/Vec3f.h"

namespace math {

template<class T>
class AABB;

enum class FrustumPlanes {
	Right,
	Left,
	Top,
	Bottom,
	Far,
	Near
};
static const uint8_t FRUSTUM_PLANES_MAX = 6;
static const uint8_t FRUSTUM_VERTICES_MAX = 8;

enum class FrustumResult {
	Outside,
	Inside,
	Intersect
};

class Frustum {
private:
	Plane _planes[FRUSTUM_PLANES_MAX];
	math::Vec3f _frustumVertices[FRUSTUM_VERTICES_MAX];

	Plane& plane(FrustumPlanes frustumPlane);
public:
	Frustum() {
		for (int i = 0; i < FRUSTUM_VERTICES_MAX; ++i) {
			_frustumVertices[i] = math::Vec3f(0.0f);
		}
	}

	Frustum(const math::Vec3f& mins, const math::Vec3f& maxs);

	void update(const math::Mat4f& mat, const math::Vec3f& mins, const math::Vec3f& maxs);

	FrustumResult test(const math::Vec3f& position) const;

	FrustumResult test(const math::Vec3f& mins, const math::Vec3f& maxs) const;

	void transform(const math::Mat4f& mat);

	bool isVisible(const math::Vec3f& mins, const math::Vec3f& maxs) const;

	bool isVisible(const math::Vec3f& pos) const;

	bool isVisible(const math::Vec3f& center, float radius) const;

	void split(const math::Mat4f& transform, math::Vec3f out[FRUSTUM_VERTICES_MAX], bool depthZeroToOne = false) const;

	void updateVertices(const math::Mat4f& view, const math::Mat4f& projection, bool depthZeroToOne = false);

	void updatePlanes(const math::Mat4f& view, const math::Mat4f& projection, bool depthZeroToOne = false);

	void update(const math::Mat4f& view, const math::Mat4f& projection, bool depthZeroToOne = false);

	AABB<float> aabb() const;

	void corners(math::Vec3f out[FRUSTUM_VERTICES_MAX], uint32_t indices[24]) const;

	const Plane& plane(FrustumPlanes frustumPlane) const;

	const Plane& operator[](size_t idx) const;

	/**
	 * @brief Checks whether a given point is within a defined frustum (2d)
	 * @param eye World position of the eye
	 * @param orientation The orientation the eye is facing to (radians)
	 * @param target World position of the target
	 * @param fieldOfView the field of view of the eye in radians
	 * @return @c true if the given target can be seen from the given eye position in that
	 * particular orientation, @c false otherwise.
	 */
	static bool isVisible(const math::Vec3f& eye, float orientation, const math::Vec3f& target, float fieldOfView);
};

inline Plane& Frustum::plane(FrustumPlanes frustumPlane) {
	return _planes[(int)frustumPlane];
}

inline const Plane& Frustum::plane(FrustumPlanes frustumPlane) const {
	return _planes[(int)frustumPlane];
}

inline void Frustum::update(const math::Mat4f& view, const math::Mat4f& projection, bool depthZeroToOne) {
	updatePlanes(view, projection, depthZeroToOne);
	updateVertices(view, projection, depthZeroToOne);
}


}
