/**
 * @file
 */

#pragma once

#include "core/GLM.h"
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
	OBB(const Vec &origin, const Vec &extents, const glm::mat4 &rotation)
		: _extents(extents), _origin(origin), _rotation(rotation) {
		_inv = glm::inverse(_rotation);
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

	bool contains(const glm::vec3 &point) const {
		const Vec min = _origin - _extents;
		const Vec max = _origin + _extents;

		const glm::vec3 &p = glm::rotate(_inv, point);
		const T x = (T)p.x;
		const T y = (T)p.y;
		const T z = (T)p.z;
		return x >= min.x && x <= max.x && y >= min.y && y <= max.y && z >= min.z && z <= max.z;
	}

	bool intersect(const glm::vec3& inRayOrigin, const glm::vec3& inRayDirection, float rayLength, float& distance) const {
		const Vec minsV = _origin - _extents;
		const Vec maxsV = _origin + _extents;

		const glm::vec3 &rayOrigin = glm::rotate(_inv, inRayOrigin);
		const glm::vec3 &rayDirection = glm::rotate(_inv, inRayDirection);
		glm::vec3 pos1;
		glm::vec3 pos2;
		double t_near = -rayLength;
		double t_far = rayLength;

		for (int i = 0; i < 3; i++) {	 // we test slabs in every direction
			if (rayDirection[i] == 0) { // ray parallel to planes in this direction
				if (rayOrigin[i] < minsV[i] || rayOrigin[i] > maxsV[i]) {
					return false; // parallel AND outside box : no intersection possible
				}
			} else { // ray not parallel to planes in this direction
				pos1[i] = (minsV[i] - rayOrigin[i]) / rayDirection[i];
				pos2[i] = (maxsV[i] - rayOrigin[i]) / rayDirection[i];

				if (pos1[i] > pos2[i]) { // we want pos1 to hold values for intersection with near plane
					core::exchange(pos1, pos2);
				}
				if (pos1[i] > t_near) {
					t_near = pos1[i];
				}
				if (pos2[i] < t_far) {
					t_far = pos2[i];
				}
				if (t_near > t_far || t_far < 0) {
					return false;
				}
			}
		}
		distance = (float)t_far;
		return true;
	}

};

} // namespace math
