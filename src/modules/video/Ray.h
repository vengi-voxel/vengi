#pragma once

#include "core/Common.h"

namespace video {

class Ray {
public:
	Ray(const glm::vec3 _origin, const glm::vec3 _direction) :
			origin(_origin), direction(_direction) {
		core_assert(!glm::any(glm::isnan(origin)));
		core_assert(!glm::any(glm::isnan(direction)));
	}

	const glm::vec3 origin;
	const glm::vec3 direction;
};

}
