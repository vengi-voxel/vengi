#include "math/Mat4f.h"
#include "math/Vec4f.h"
#include "math/Vec3f.h"
/**
 * @file
 */

#pragma once


namespace math {

enum class PlaneSide {
	Front, Back, On
};

class Plane {
private:
	math::Vec3f _norm {0.0f};
	float _dist = 0.0f;
public:
	Plane();
	Plane(const math::Vec4f& data);
	Plane(const math::Vec3f& norm, float dist);
	Plane(const math::Vec3f& norm, const math::Vec3f& point);
	Plane(const math::Vec3f& point1, const math::Vec3f& point2, const math::Vec3f& point3);

	void set(const math::Vec4f& data);
	void set(const math::Vec3f& norm, float dist);
	void set(const math::Vec3f& norm, const math::Vec3f& point);
	void set(const math::Vec3f& point1, const math::Vec3f& point2, const math::Vec3f& point3);

	static bool intersectPlanes(const math::Plane& p1, const math::Plane& p2, const math::Plane& p3, math::Vec3f &intersection);

	void transform(const math::Mat4f& mat);

	void normalize();

	float dist() const;
	const math::Vec3f& norm() const;
	PlaneSide side(const math::Vec3f& point) const;

	bool isBackSide(const math::Vec3f& point) const;
	bool isFrontSide(const math::Vec3f& point) const;

	float distanceToPlane(const math::Vec3f& point) const;
};

inline Plane::Plane(const math::Vec3f& norm, float dist) {
	set(norm, dist);
}

inline Plane::Plane(const math::Vec3f& norm, const math::Vec3f& point) {
	set(norm, point);
}

inline Plane::Plane(const math::Vec3f& point1, const math::Vec3f& point2, const math::Vec3f& point3) {
	set(point1, point2, point3);
}

inline void Plane::set(const math::Vec3f& norm, float dist) {
	_norm = norm;
	_dist = -dist;
}

inline const math::Vec3f& Plane::norm() const {
	return _norm;
}

inline bool Plane::isBackSide(const math::Vec3f& point) const {
	return distanceToPlane(point) < 0.0f;
}

inline bool Plane::isFrontSide(const math::Vec3f& point) const {
	return distanceToPlane(point) > 0.0f;
}

inline float Plane::dist() const {
	return _dist;
}

}

