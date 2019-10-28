/**
 * @file
 */

#include "Frustum.h"
#include "core/Common.h"
#include "core/Trace.h"

namespace math {

static const glm::vec4 cornerVecs[FRUSTUM_VERTICES_MAX] = {
	glm::vec4(-1.0f,  1.0f,  1.0f, 1.0f), glm::vec4(-1.0f, -1.0f,  1.0f, 1.0f),
	glm::vec4( 1.0f,  1.0f,  1.0f, 1.0f), glm::vec4( 1.0f, -1.0f,  1.0f, 1.0f),
	glm::vec4(-1.0f,  1.0f, -1.0f, 1.0f), glm::vec4(-1.0f, -1.0f, -1.0f, 1.0f),
	glm::vec4( 1.0f,  1.0f, -1.0f, 1.0f), glm::vec4( 1.0f, -1.0f, -1.0f, 1.0f)
};

void Frustum::transform(const glm::mat4& mat) {
	for (uint8_t i = 0; i < FRUSTUM_PLANES_MAX; ++i) {
		_planes[i].transform(mat);
	}
}

void Frustum::corners(glm::vec3 out[FRUSTUM_VERTICES_MAX], uint32_t indices[24]) const {
	if (out != nullptr) {
		for (uint8_t i = 0; i < FRUSTUM_VERTICES_MAX; ++i) {
			out[i] = _frustumVertices[i];
		}
	}

	if (indices == nullptr) {
		return;
	}

	uint32_t currentIndex = 0;

	// near plane
	indices[currentIndex++] = 0;
	indices[currentIndex++] = 1;

	indices[currentIndex++] = 1;
	indices[currentIndex++] = 3;

	indices[currentIndex++] = 3;
	indices[currentIndex++] = 2;

	indices[currentIndex++] = 2;
	indices[currentIndex++] = 0;

	// far plane
	indices[currentIndex++] = 4;
	indices[currentIndex++] = 5;

	indices[currentIndex++] = 5;
	indices[currentIndex++] = 7;

	indices[currentIndex++] = 7;
	indices[currentIndex++] = 6;

	indices[currentIndex++] = 6;
	indices[currentIndex++] = 4;

	// connections
	indices[currentIndex++] = 0;
	indices[currentIndex++] = 4;

	indices[currentIndex++] = 2;
	indices[currentIndex++] = 6;

	indices[currentIndex++] = 1;
	indices[currentIndex++] = 5;

	indices[currentIndex++] = 3;
	indices[currentIndex++] = 7;
}

math::AABB<float> Frustum::aabb() const {
	return math::AABB<float>::construct(_frustumVertices, FRUSTUM_VERTICES_MAX);
}

void Frustum::split(const glm::mat4& transform, glm::vec3 out[FRUSTUM_VERTICES_MAX]) const {
	for (uint8_t i = 0; i < FRUSTUM_VERTICES_MAX; ++i) {
		const glm::vec4& v = transform * cornerVecs[i];
		out[i] = glm::vec3(v) / v.w;
		core_assert(!glm::any(glm::isnan(out[i])));
	}
}

/**
 * http://gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf
 * https://fgiesen.wordpress.com/2012/08/31/frustum-planes-from-the-projection-matrix/
 */
void Frustum::updatePlanes(const glm::mat4& view, const glm::mat4& projection) {
	// world space
	const glm::mat4 &clipMatrix = projection * view;

	const glm::vec4& rx = glm::row(clipMatrix, 0);
	const glm::vec4& ry = glm::row(clipMatrix, 1);
	const glm::vec4& rz = glm::row(clipMatrix, 2);
	const glm::vec4& rw = glm::row(clipMatrix, 3);

	plane(FrustumPlanes::Right). set(rw + rx);
	plane(FrustumPlanes::Left).  set(rw - rx);
	plane(FrustumPlanes::Bottom).set(rw + ry);
	plane(FrustumPlanes::Top).   set(rw - ry);
	plane(FrustumPlanes::Far).   set(rw + rz);
	plane(FrustumPlanes::Near).  set(rw - rz);
}

void Frustum::updateVertices(const glm::mat4& view, const glm::mat4& projection) {
	const glm::mat4& transform = glm::inverse(projection * view);
	for (uint8_t i = 0; i < FRUSTUM_VERTICES_MAX; ++i) {
		const glm::vec4& v = transform * cornerVecs[i];
		_frustumVertices[i] = glm::vec3(v) / v.w;
		core_assert(!glm::any(glm::isnan(_frustumVertices[i])));
		core_assert(!glm::any(glm::isinf(_frustumVertices[i])));
	}
}

FrustumResult Frustum::test(const glm::vec3& position) const {
	FrustumResult result = FrustumResult::Inside;
	for (uint8_t i = 0; i < FRUSTUM_PLANES_MAX; i++) {
		const Plane& p = _planes[i];
		if (p.isBackSide(position)) {
			return FrustumResult::Outside;
		}
	}
	return result;
}

bool Frustum::isVisible(const glm::vec3& mins, const glm::vec3& maxs) const {
	core_trace_scoped(FrustumIsVisible);
	for (uint8_t i = 0; i < FRUSTUM_PLANES_MAX; ++i) {
		const Plane& p = _planes[i];
		const glm::vec3& normal = p.norm();

		const glm::vec3 pos(normal.x > 0.0f ? maxs.x : mins.x,
				normal.y > 0.0f ? maxs.y : mins.y,
				normal.z > 0.0f ? maxs.z : mins.z);

		if (p.isBackSide(pos)) {
			return false;
		}
	}

	return true;
}

bool Frustum::isVisible(const glm::vec3& center, float radius) const {
	for (uint8_t i = 0; i < FRUSTUM_PLANES_MAX; ++i) {
		const Plane& p = _planes[i];
		const float dist = p.distanceToPlane(center);
		if (-dist > radius) {
			return false;
		}
	}

	return true;
}

bool Frustum::isVisible(const glm::vec3& pos) const {
	for (uint8_t i = 0; i < FRUSTUM_PLANES_MAX; ++i) {
		const Plane& p = _planes[i];
		const bool back = p.isBackSide(pos);
		if (back) {
			return false;
		}
	}

	return true;
}

FrustumResult Frustum::test(const glm::vec3& mins, const glm::vec3& maxs) const {
	core_trace_scoped(FrustumTest);
	FrustumResult result = FrustumResult::Inside;
	for (uint8_t i = 0; i < FRUSTUM_PLANES_MAX; ++i) {
		const Plane& p = _planes[i];
		const glm::vec3& normal = p.norm();

		const glm::vec3 positiveVertex(normal.x > 0.0f ? maxs.x : mins.x,
				normal.y > 0.0f ? maxs.y : mins.y,
				normal.z > 0.0f ? maxs.z : mins.z);

		if (p.isBackSide(positiveVertex)) {
			return FrustumResult::Outside;
		}

		const glm::vec3 negativeVertex(normal.x > 0.0f ? mins.x : maxs.x,
				normal.y > 0.0f ? mins.y : maxs.y,
				normal.z > 0.0f ? mins.z : maxs.z);
		if (p.isBackSide(negativeVertex)) {
			result = FrustumResult::Intersect;
		}
	}

	return result;
}

bool Frustum::isVisible(const glm::vec3& eye, float orientation, const glm::vec3& target, float fieldOfView) {
	const glm::vec2 direction = glm::normalize(glm::vec2(target.x - eye.x, target.z - eye.z));
	const float angle = ::atan2(direction.y, direction.x);

	float delta;
	if (angle > orientation) {
		delta = angle - orientation;
	} else {
		delta = orientation - angle;
	}
	return delta <= fieldOfView;
}

}
