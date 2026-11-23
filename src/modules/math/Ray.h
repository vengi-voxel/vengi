/**
 * @file
 */
#pragma once

#include "core/Common.h"
#include "math/Vec3f.h"
#include "math/Scalar.h"

namespace math {

/**
 * @brief Defines origin and direction of a ray
 */
class Ray {
public:
	Ray(const math::Vec3f& _origin, const math::Vec3f& _direction) :
			origin(_origin), direction(_direction) {
	}

	inline bool isValid() const {
		return !math::any(math::isnan(origin)) && !math::any(math::isnan(direction));
	}

	math::Vec3f origin;
	math::Vec3f direction;
};

}
