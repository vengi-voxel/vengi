#pragma once

#include "core/GLM.h"

namespace core {

enum class PlaneSide {
	Front, Back, On
};

class Plane {
private:
	glm::vec3 _norm;
	float _dist = 0.0f;
public:
	Plane();
	Plane(const glm::vec4& data);
	Plane(const glm::vec3& norm, float dist);
	Plane(const glm::vec3& norm, const glm::vec3& point);
	Plane(const glm::vec3& point1, const glm::vec3& point2, const glm::vec3& point3);

	void set(const glm::vec4& data);
	void set(const glm::vec3& norm, float dist);
	void set(const glm::vec3& norm, const glm::vec3& point);
	void set(const glm::vec3& point1, const glm::vec3& point2, const glm::vec3& point3);

	void transform(const glm::mat4& mat);

	void normalize();

	float dist() const;
	const glm::vec3& norm() const;
	PlaneSide side(const glm::vec3& point) const;

	bool isBackSide(const glm::vec3& point) const;
	bool isFrontSide(const glm::vec3& point) const;

	float distanceToPlane(const glm::vec3& point) const;
};

inline Plane::Plane(const glm::vec4& data) {
	set(data);
}

inline Plane::Plane(const glm::vec3& norm, float dist) {
	set(norm, dist);
}

inline Plane::Plane(const glm::vec3& norm, const glm::vec3& point) {
	set(norm, point);
}

inline Plane::Plane(const glm::vec3& point1, const glm::vec3& point2, const glm::vec3& point3) {
	set(point1, point2, point3);
}

inline void Plane::transform(const glm::mat4& mat) {
	const glm::vec3& n = glm::rotate(mat, norm());
	const glm::vec3& p = glm::transform(mat, norm() * dist());
	set(n, p);
}

inline void Plane::set(const glm::vec4& data) {
	_norm = data.xyz();
	_dist = data.w;
}

inline void Plane::set(const glm::vec3& norm, float dist) {
	_norm = norm;
	_dist = -dist;
}

inline void Plane::set(const glm::vec3& norm, const glm::vec3& point) {
	set(norm, glm::dot(norm, point));
}

inline const glm::vec3& Plane::norm() const {
	return _norm;
}

inline float Plane::distanceToPlane(const glm::vec3& point) const {
	return glm::dot(norm(), point) + dist();
}

inline bool Plane::isBackSide(const glm::vec3& point) const {
	return distanceToPlane(point) < 0.0f;
}

inline bool Plane::isFrontSide(const glm::vec3& point) const {
	return distanceToPlane(point) > 0.0f;
}

inline float Plane::dist() const {
	return _dist;
}

}

