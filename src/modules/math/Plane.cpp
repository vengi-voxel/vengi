/**
 * @file
 */

#include "Plane.h"
#include "math/Mat4f.h"
#include "math/Vec4f.h"
#include "math/Vec3f.h"
#include "math/Scalar.h"

namespace math {

Plane::Plane()  {
}

Plane::Plane(const math::Vec4f& data) {
	set(data);
}

void Plane::set(const math::Vec4f& data) {
	_norm = math::Vec3f(data);
	_dist = data.w;
}

void Plane::normalize() {
	const float length = math::length(norm());
	if (length > 0.0f) {
		const float scale = 1.0f / length;
		_norm *= scale;
		_dist *= scale;
	}
}

void Plane::set(const math::Vec3f& point1, const math::Vec3f& point2, const math::Vec3f& point3) {
	const math::Vec3f& delta1 = point2 - point1;
	const math::Vec3f& delta2 = point3 - point1;
	const math::Vec3f& norm = math::cross(delta2, delta1);
	set(norm, point1);
}

PlaneSide Plane::side(const math::Vec3f& point) const {
	const float dist = distanceToPlane(point);

	if (dist > math::epsilonf) {
		return PlaneSide::Front;
	}

	if (dist < -math::epsilonf) {
		return PlaneSide::Back;
	}

	return PlaneSide::On;
}

void Plane::transform(const math::Mat4f& mat) {
	const math::Vec3f& n = math::rotate(mat, norm());
	const math::Vec3f& p = math::transform(mat, norm() * dist());
	set(n, p);
}

void Plane::set(const math::Vec3f& norm, const math::Vec3f& point) {
	set(norm, math::dot(norm, point));
}

float Plane::distanceToPlane(const math::Vec3f& point) const {
	return math::dot(norm(), point) + dist();
}

bool Plane::intersectPlanes(const math::Plane& p1, const math::Plane& p2, const math::Plane& p3, math::Vec3f &intersection) {
	const math::Vec3f& n1 = p1.norm();
	const math::Vec3f& n2 = p2.norm();
	const math::Vec3f& n3 = p3.norm();

	const math::Vec3f& n1n2 = math::cross(n1, n2);
	const math::Vec3f& n2n3 = math::cross(n2, n3);
	const math::Vec3f& n3n1 = math::cross(n3, n1);

	const float denom = math::dot(n1, n2n3);
	if (math::abs(denom) < math::epsilonf) {
		return false;
	}

	const math::Vec3f& temp1 = n2n3 * -p1.dist();
	const math::Vec3f& temp2 = n3n1 * -p2.dist();
	const math::Vec3f& temp3 = n1n2 * -p3.dist();
	intersection = (temp1 - temp2 - temp3) / denom;
	return true;
}

} // namespace math
