/**
 * @file
 */

#pragma once

#include "core/GLM.h"
#include "core/Pair.h"
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace math {

template <class T>
class OBB {
private:
	using Vec = glm::vec<3, T, glm::defaultp>;
	Vec _extents;
	Vec _origin;
	glm::mat4 _rotation;
	glm::mat4 _inv;

public:
	/**
	 * @brief Construct a new OBB object
	 *
	 * @param origin The position
	 * @param extents The half size
	 * @param rotation The rotation of the object (the translation is already part of the position)
	 */
	OBB(const Vec &origin, const Vec &extents, const glm::mat3x3 &rotation)
		: _extents(extents), _origin(origin), _rotation(rotation) {
		_inv = glm::inverse(_rotation);
	}

	OBB(const Vec &origin, const Vec &pivot, const Vec &extents, const glm::mat3x3 &rotation)
		: _extents(extents), _origin(origin), _rotation(rotation) {
		_inv = glm::inverse(_rotation);
		const glm::vec3 &rotatedP = _rotation * glm::vec4(pivot, 1.0f);
		_origin -= rotatedP;
	}

	OBB(const Vec &mins, const Vec &maxs) : _extents((maxs - mins) / (T)2), _origin(mins + _extents), _rotation(1.0f) {
		_inv = glm::inverse(_rotation);
	}

	void setRotation(const glm::mat4 &mat) {
		_rotation = mat;
		_inv = glm::inverse(_rotation);
	}

	void setOrigin(const Vec &origin) {
		_origin = origin;
	}

	void setExtents(const Vec &extents) {
		_extents = extents;
	}

	const Vec &origin() const {
		return _origin;
	}

	const Vec &extents() const {
		return _extents;
	}

	const glm::mat4 &rotation() const {
		return _rotation;
	}

	T width() const {
		return _extents.x * (T)2;
	}

	T height() const {
		return _extents.y * (T)2;
	}

	T depth() const {
		return _extents.z * (T)2;
	}

	core::Pair<Vec, Vec> bounds() const {
		Vec corners[] = {Vec(-_extents.x, -_extents.y, -_extents.z), Vec(_extents.x, -_extents.y, -_extents.z),
						 Vec(-_extents.x, _extents.y, -_extents.z),	 Vec(_extents.x, _extents.y, -_extents.z),
						 Vec(-_extents.x, -_extents.y, _extents.z),	 Vec(_extents.x, -_extents.y, _extents.z),
						 Vec(-_extents.x, _extents.y, _extents.z),	 Vec(_extents.x, _extents.y, _extents.z)};
		for (int i = 0; i < 8; i++) {
			corners[i] = _origin + glm::vec3(_rotation * glm::vec4(corners[i], 1.0f));
		}
		Vec minCoords(FLT_MAX);
		Vec maxCoords(-FLT_MAX);
		for (int i = 0; i < 8; i++) {
			minCoords = glm::min(minCoords, corners[i]);
			maxCoords = glm::max(maxCoords, corners[i]);
		}
		return core::Pair<Vec, Vec>(minCoords, maxCoords);
	}

	bool contains(const glm::vec3 &point) const {
		const Vec min = _origin - _extents;
		const Vec max = _origin + _extents;

		const glm::vec4 &p = _inv * glm::vec4(point, 1.0f);
		const T x = (T)p.x;
		const T y = (T)p.y;
		const T z = (T)p.z;
		return x >= min.x && x <= max.x && y >= min.y && y <= max.y && z >= min.z && z <= max.z;
	}

	/**
	 * @param[out] distance the distance to the hit point. Only valid if the method returns true.
	 */
	bool intersect(const glm::vec3 &rayOrigin, const glm::vec3 &rayDirection, float rayLength, float &distance) const {
		// Transform the ray into the OBB's local space
		const glm::vec3 localRayOrigin = _inv * glm::vec4(rayOrigin - _origin, 1.0f);
		const glm::vec3 localRayDirection = _inv * glm::vec4(rayDirection, 1.0f);

		// Calculate the intersections with the local AABB
		const glm::vec3 tMin = (-_extents - localRayOrigin) / localRayDirection;
		const glm::vec3 tMax = (_extents - localRayOrigin) / localRayDirection;

		const glm::vec3 realTMin = glm::min(tMin, tMax);
		const glm::vec3 realTMax = glm::max(tMin, tMax);

		const float maxTMin = glm::max(glm::max(realTMin.x, realTMin.y), realTMin.z);
		const float minTMax = glm::min(glm::min(realTMax.x, realTMax.y), realTMax.z);

		if (minTMax < maxTMin) {
			return false;
		}

		distance = maxTMin;

		return true;
	}
};

} // namespace math
