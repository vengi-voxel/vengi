/**
 * @file
 */

#include "MoveVector.h"
#include "core/GLM.h"
#include <glm/gtc/constants.hpp>
#include <math.h>

namespace backend {

const MoveVector MoveVector::Invalid(glm::vec3(), 0.0f, false);

MoveVector::MoveVector(const glm::vec3& vec3, float rotation, bool valid) :
		_vec3(vec3), _rotation(rotation), _valid(valid) {
	if (valid) {
		glm_assert_vec3(_vec3);
	}
}

float MoveVector::getOrientation(float duration) const {
	const float pi2 = glm::two_pi<float>();
	const float rotation = _rotation + pi2;
	return fmodf(rotation * duration, pi2);
}

}
