#include "Plane.h"

namespace core {

Plane::Plane()  {
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

}
