/**
 * @file
 */

#pragma once

#include "Plane.h"
#include "AABB.h"
#include <stdint.h>

namespace math {

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
	glm::vec3 _frustumVertices[FRUSTUM_VERTICES_MAX];

	Plane& plane(FrustumPlanes frustumPlane);
public:
	Frustum() {
		for (int i = 0; i < FRUSTUM_VERTICES_MAX; ++i) {
			_frustumVertices[i] = glm::vec3(0.0f);
		}
	}

	template<class T>
	Frustum(const math::AABB<T>& aabb) {
		const glm::vec3 mins(aabb.mins());
		const glm::vec3 maxs(aabb.maxs());
		update(glm::mat4(1.0f), mins, maxs);
	}

	void update(const glm::mat4& mat, const glm::vec3& mins, const glm::vec3& maxs);

	FrustumResult test(const glm::vec3& position) const;

	FrustumResult test(const glm::vec3& mins, const glm::vec3& maxs) const;

	void transform(const glm::mat4& mat);

	bool isVisible(const glm::vec3& mins, const glm::vec3& maxs) const;

	bool isVisible(const glm::vec3& pos) const;

	bool isVisible(const glm::vec3& center, float radius) const;

	void split(const glm::mat4& transform, glm::vec3 out[FRUSTUM_VERTICES_MAX]) const;

	void updateVertices(const glm::mat4& view, const glm::mat4& projection);

	void updatePlanes(const glm::mat4& view, const glm::mat4& projection);

	void update(const glm::mat4& view, const glm::mat4& projection);

	math::AABB<float> aabb() const;

	void corners(glm::vec3 out[FRUSTUM_VERTICES_MAX], uint32_t indices[24]) const;

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
	static bool isVisible(const glm::vec3& eye, float orientation, const glm::vec3& target, float fieldOfView);
};

inline Plane& Frustum::plane(FrustumPlanes frustumPlane) {
	return _planes[(int)frustumPlane];
}

inline const Plane& Frustum::plane(FrustumPlanes frustumPlane) const {
	return _planes[(int)frustumPlane];
}

inline void Frustum::update(const glm::mat4& view, const glm::mat4& projection) {
	updatePlanes(view, projection);
	updateVertices(view, projection);
}


}
