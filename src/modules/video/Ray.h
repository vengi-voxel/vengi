#pragma once

namespace video {

class Ray {
public:
	Ray(const glm::vec3 _origin, const glm::vec3 _direction) :
			origin(_origin), direction(_direction) {
	}

	const glm::vec3 origin;
	const glm::vec3 direction;
};

}
