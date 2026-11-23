/**
 * @file
 */

#pragma once

#include <float.h>
#include "math/Mat4f.h"
#include "math/Mat3f.h"
#include "math/Vec4f.h"
#include "math/VecOf.h"

namespace math {

void obbSetInverse(Mat4f &inv, const Mat4f &rotation);

template <class T>
class OBB {
private:
	using Vec = typename Vec3Of<T>::Type;
	Vec _extents;
	Vec _origin;
	math::Mat4f _rotation;
	math::Mat4f _inv;

public:
	/**
	 * @brief Construct a new OBB object
	 *
	 * @param origin The position
	 * @param extents The half size (unscaled - scale is given by the matrix)
	 * @param rotation The rotation of the object (the translation is already part of the position)
	 */
	OBB(const Vec &origin, const Vec &extents, const math::Mat3f &rotation)
		: _extents(extents), _origin(origin), _rotation(rotation) {
		obbSetInverse(_inv, _rotation);
	}

	OBB(const Vec &origin, const Vec &pivot, const Vec &extents, const math::Mat3f &rotation)
		: _extents(extents), _origin(origin), _rotation(rotation) {
		obbSetInverse(_inv, _rotation);
		const math::Vec3f &rotatedP = _rotation * math::Vec4f(pivot, 1.0f);
		_origin -= rotatedP;
	}

	OBB(const Vec &mins, const Vec &maxs) : _extents((maxs - mins) / (T)2), _origin(mins + _extents), _rotation(1.0f) {
		obbSetInverse(_inv, _rotation);
	}

	void setRotation(const math::Mat3f &mat) {
		_rotation = mat;
		obbSetInverse(_inv, _rotation);
	}

	void setOrigin(const Vec &origin) {
		_origin = origin;
	}

	// unscaled extents
	void setExtents(const Vec &extents) {
		_extents = extents;
	}

	const Vec &origin() const {
		return _origin;
	}

	// unscaled extents
	const Vec &extents() const {
		return _extents;
	}

	const math::Mat4f &rotation() const {
		return _rotation;
	}

	bool contains(const math::Vec3f &point) const {
		const Vec min = _origin - _extents;
		const Vec max = _origin + _extents;

		const math::Vec4f &p = _inv * math::Vec4f(point, 1.0f);
		const T x = (T)p.x;
		const T y = (T)p.y;
		const T z = (T)p.z;
		return x >= min.x && x <= max.x && y >= min.y && y <= max.y && z >= min.z && z <= max.z;
	}

	/**
	 * @param[out] distance the distance to the hit point. Only valid if the method returns true.
	 */
	bool intersect(const math::Vec3f &rayOrigin, const math::Vec3f &rayDirection, float &distance) const {
		// Transform the ray into the OBB's local space
		const math::Vec3f localRayOrigin = _inv * math::Vec4f(rayOrigin - _origin, 1.0f);
		const math::Vec3f localRayDirection = _inv * math::Vec4f(rayDirection, 1.0f);

		// Calculate the intersections with the local AABB
		const math::Vec3f tMin = (-_extents - localRayOrigin) / localRayDirection;
		const math::Vec3f tMax = (_extents - localRayOrigin) / localRayDirection;

		const math::Vec3f realTMin = math::min(tMin, tMax);
		const math::Vec3f realTMax = math::max(tMin, tMax);

		const float maxTMin = math::max(math::max(realTMin.x, realTMin.y), realTMin.z);
		const float minTMax = math::min(math::min(realTMax.x, realTMax.y), realTMax.z);

		if (minTMax < maxTMin) {
			return false;
		}

		distance = maxTMin;

		return true;
	}
};

using OBBF = OBB<float>;

} // namespace math
