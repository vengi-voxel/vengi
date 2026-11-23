/**
 * @file
 */

#include "Frustum.h"
#include "core/TraceScoped.h"
#include "core/Assert.h"
#include "math/AABB.h"
#include "math/Mat4f.h"
#include "math/Vec4f.h"
#include "math/Vec3f.h"
#include "math/Vec2f.h"
#include "math/Mat4Ops.h"
#include "math/Scalar.h"

namespace math {

Frustum::Frustum(const math::Vec3f& mins, const math::Vec3f& maxs) {
	update(math::Mat4f(1.0f), mins, maxs);
}

void Frustum::update(const math::Mat4f& mat, const math::Vec3f& mins, const math::Vec3f& maxs) {
	update(mat, math::ortho(mins.x, maxs.x, mins.y, maxs.y, -mins.z, -maxs.z));
}

const Plane& Frustum::operator[](size_t idx) const {
	core_assert(idx < FRUSTUM_PLANES_MAX);
	return _planes[idx];
}

void Frustum::transform(const math::Mat4f& mat) {
	for (uint8_t i = 0; i < FRUSTUM_PLANES_MAX; ++i) {
		_planes[i].transform(mat);
	}
}

void Frustum::corners(math::Vec3f out[FRUSTUM_VERTICES_MAX], uint32_t indices[24]) const {
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

void Frustum::split(const math::Mat4f& transform, math::Vec3f out[FRUSTUM_VERTICES_MAX], bool depthZeroToOne) const {
	const float nearZ = depthZeroToOne ? 0.0f : -1.0f;
	const float farZ = 1.0f;
	static const math::Vec4f cornerVecs[FRUSTUM_VERTICES_MAX] = {
		math::Vec4f(-1.0f,  1.0f,  farZ, 1.0f), math::Vec4f(-1.0f, -1.0f,  farZ, 1.0f),
		math::Vec4f( 1.0f,  1.0f,  farZ, 1.0f), math::Vec4f( 1.0f, -1.0f,  farZ, 1.0f),
		math::Vec4f(-1.0f,  1.0f, nearZ, 1.0f), math::Vec4f(-1.0f, -1.0f, nearZ, 1.0f),
		math::Vec4f( 1.0f,  1.0f, nearZ, 1.0f), math::Vec4f( 1.0f, -1.0f, nearZ, 1.0f)
	};

	for (uint8_t i = 0; i < FRUSTUM_VERTICES_MAX; ++i) {
		const math::Vec4f& v = transform * cornerVecs[i];
		out[i] = math::Vec3f(v) / v.w;
		core_assert(!math::any(math::isnan(out[i])));
	}
}

/**
 * http://gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf
 * https://fgiesen.wordpress.com/2012/08/31/frustum-planes-from-the-projection-matrix/
 */
void Frustum::updatePlanes(const math::Mat4f& view, const math::Mat4f& projection, bool depthZeroToOne) {
	// world space
	const math::Mat4f &clipMatrix = projection * view;

	const math::Vec4f& rx = math::row(clipMatrix, 0);
	const math::Vec4f& ry = math::row(clipMatrix, 1);
	const math::Vec4f& rz = math::row(clipMatrix, 2);
	const math::Vec4f& rw = math::row(clipMatrix, 3);

	plane(FrustumPlanes::Right). set(rw + rx);
	plane(FrustumPlanes::Left).  set(rw - rx);
	plane(FrustumPlanes::Bottom).set(rw + ry);
	plane(FrustumPlanes::Top).   set(rw - ry);
	if (depthZeroToOne) {
		plane(FrustumPlanes::Near).set(rz);
		plane(FrustumPlanes::Far). set(rw - rz);
	} else {
		plane(FrustumPlanes::Far). set(rw + rz);
		plane(FrustumPlanes::Near).set(rw - rz);
	}
}

void Frustum::updateVertices(const math::Mat4f& view, const math::Mat4f& projection, bool depthZeroToOne) {
	const math::Mat4f& transform = math::inverse(projection * view);
	const float nearZ = depthZeroToOne ? 0.0f : -1.0f;
	const float farZ = 1.0f;
	static const math::Vec4f cornerVecs[FRUSTUM_VERTICES_MAX] = {
		math::Vec4f(-1.0f,  1.0f,  farZ, 1.0f), math::Vec4f(-1.0f, -1.0f,  farZ, 1.0f),
		math::Vec4f( 1.0f,  1.0f,  farZ, 1.0f), math::Vec4f( 1.0f, -1.0f,  farZ, 1.0f),
		math::Vec4f(-1.0f,  1.0f, nearZ, 1.0f), math::Vec4f(-1.0f, -1.0f, nearZ, 1.0f),
		math::Vec4f( 1.0f,  1.0f, nearZ, 1.0f), math::Vec4f( 1.0f, -1.0f, nearZ, 1.0f)
	};

	for (uint8_t i = 0; i < FRUSTUM_VERTICES_MAX; ++i) {
		const math::Vec4f& v = transform * cornerVecs[i];
		_frustumVertices[i] = v / v.w;
		core_assert(!math::any(math::isnan(_frustumVertices[i])));
		core_assert(!math::any(math::isinf(_frustumVertices[i])));
	}
}

FrustumResult Frustum::test(const math::Vec3f& position) const {
	FrustumResult result = FrustumResult::Inside;
	for (uint8_t i = 0; i < FRUSTUM_PLANES_MAX; ++i) {
		const Plane& p = _planes[i];
		if (p.isBackSide(position)) {
			return FrustumResult::Outside;
		}
	}
	return result;
}

bool Frustum::isVisible(const math::Vec3f& mins, const math::Vec3f& maxs) const {
	for (uint8_t i = 0; i < FRUSTUM_PLANES_MAX; ++i) {
		const Plane& p = _planes[i];
		const math::Vec3f& normal = p.norm();

		const math::Vec3f pos(normal.x > 0.0f ? maxs.x : mins.x,
				normal.y > 0.0f ? maxs.y : mins.y,
				normal.z > 0.0f ? maxs.z : mins.z);

		if (p.isBackSide(pos)) {
			return false;
		}
	}

	return true;
}

bool Frustum::isVisible(const math::Vec3f& center, float radius) const {
	for (uint8_t i = 0; i < FRUSTUM_PLANES_MAX; ++i) {
		const Plane& p = _planes[i];
		const float dist = p.distanceToPlane(center);
		if (-dist > radius) {
			return false;
		}
	}

	return true;
}

bool Frustum::isVisible(const math::Vec3f& pos) const {
	for (uint8_t i = 0; i < FRUSTUM_PLANES_MAX; ++i) {
		const Plane& p = _planes[i];
		const bool back = p.isBackSide(pos);
		if (back) {
			return false;
		}
	}

	return true;
}

FrustumResult Frustum::test(const math::Vec3f& mins, const math::Vec3f& maxs) const {
	core_trace_scoped(FrustumTest);
	FrustumResult result = FrustumResult::Inside;
	for (uint8_t i = 0; i < FRUSTUM_PLANES_MAX; ++i) {
		const Plane& p = _planes[i];
		const math::Vec3f& normal = p.norm();

		const math::Vec3f positiveVertex(normal.x > 0.0f ? maxs.x : mins.x,
				normal.y > 0.0f ? maxs.y : mins.y,
				normal.z > 0.0f ? maxs.z : mins.z);

		if (p.isBackSide(positiveVertex)) {
			return FrustumResult::Outside;
		}

		const math::Vec3f negativeVertex(normal.x > 0.0f ? mins.x : maxs.x,
				normal.y > 0.0f ? mins.y : maxs.y,
				normal.z > 0.0f ? mins.z : maxs.z);
		if (p.isBackSide(negativeVertex)) {
			result = FrustumResult::Intersect;
		}
	}

	return result;
}

bool Frustum::isVisible(const math::Vec3f& eye, float orientation, const math::Vec3f& target, float fieldOfView) {
	const math::Vec2f direction = math::normalize(math::Vec2f(target.x - eye.x, target.z - eye.z));
	const float angle = math::atan(direction.y, direction.x);

	float delta;
	if (angle > orientation) {
		delta = angle - orientation;
	} else {
		delta = orientation - angle;
	}
	return delta <= fieldOfView;
}

}
