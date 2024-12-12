/**
 * @file
 */

#include "Plane.h"
#include "core/GLM.h"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace math {

Plane::Plane()  {
}

Plane::Plane(const glm::vec4& data) {
	set(data);
}

void Plane::set(const glm::vec4& data) {
	_norm = glm::vec3(data);
	_dist = data.w;
}

void Plane::normalize() {
	const float length = glm::length(norm());
	if (length > 0.0f) {
		const float scale = 1.0f / length;
		_norm *= scale;
		_dist *= scale;
	}
}

void Plane::set(const glm::vec3& point1, const glm::vec3& point2, const glm::vec3& point3) {
	const glm::vec3& delta1 = point2 - point1;
	const glm::vec3& delta2 = point3 - point1;
	const glm::vec3& norm = glm::cross(delta2, delta1);
	set(norm, point1);
}

PlaneSide Plane::side(const glm::vec3& point) const {
	const float dist = distanceToPlane(point);

	if (dist > glm::epsilon<float>()) {
		return PlaneSide::Front;
	}

	if (dist < -glm::epsilon<float>()) {
		return PlaneSide::Back;
	}

	return PlaneSide::On;
}

void Plane::transform(const glm::mat4& mat) {
	const glm::vec3& n = glm::rotate(mat, norm());
	const glm::vec3& p = glm::transform(mat, norm() * dist());
	set(n, p);
}

void Plane::set(const glm::vec3& norm, const glm::vec3& point) {
	set(norm, glm::dot(norm, point));
}

float Plane::distanceToPlane(const glm::vec3& point) const {
	return glm::dot(norm(), point) + dist();
}

bool Plane::intersectPlanes(const math::Plane& p1, const math::Plane& p2, const math::Plane& p3, glm::vec3 &intersection) {
	const glm::vec3& n1 = p1.norm();
	const glm::vec3& n2 = p2.norm();
	const glm::vec3& n3 = p3.norm();

	const glm::vec3& n1n2 = glm::cross(n1, n2);
	const glm::vec3& n2n3 = glm::cross(n2, n3);
	const glm::vec3& n3n1 = glm::cross(n3, n1);

	const float denom = glm::dot(n1, n2n3);
	if (glm::abs(denom) < glm::epsilon<float>()) {
		return false;
	}

	const glm::vec3& temp1 = n2n3 * -p1.dist();
	const glm::vec3& temp2 = n3n1 * -p2.dist();
	const glm::vec3& temp3 = n1n2 * -p3.dist();
	intersection = (temp1 - temp2 - temp3) / denom;
	return true;
}

} // namespace math
