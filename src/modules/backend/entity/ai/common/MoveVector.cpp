/**
 * @file
 */

#include "MoveVector.h"
#include <glm/gtc/constants.hpp>
#include <math.h>

namespace backend {

MoveVector::MoveVector(const glm::vec3& vec3, float rotation) :
		_vec3(vec3), _rotation(rotation) {
}

MoveVector::MoveVector(const glm::vec3& vec3, double rotation) :
		_vec3(vec3), _rotation(static_cast<float>(rotation)) {
}

float MoveVector::getOrientation(float duration) const {
	const float pi2 = glm::two_pi<float>();
	const float rotation = _rotation + pi2;
	return fmodf(rotation * duration, pi2);
}

}
