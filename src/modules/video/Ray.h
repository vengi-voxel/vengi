/**
 * @file
 */
#pragma once

#include "core/Common.h"
#include <glm/common.hpp>
#include <glm/vector_relational.hpp>
#include <glm/vec3.hpp>

namespace video {

/**
 * @brief Defines origin and direction of a ray
 * @ingroup Video
 */
class Ray {
public:
	Ray(const glm::vec3& _origin, const glm::vec3& _direction) :
			origin(_origin), direction(_direction) {
	}

	inline bool isValid() const {
		return !glm::any(glm::isnan(origin)) && !glm::any(glm::isnan(direction));
	}

	glm::vec3 origin;
	glm::vec3 direction;
};

}
