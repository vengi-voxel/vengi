/**
 * @file
 */

#include "MoveVector.h"
#include "core/GLM.h"
#include <glm/common.hpp>
#include <glm/gtc/constants.hpp>

namespace backend {

const MoveVector MoveVector::Invalid(glm::vec3(), 0.0f, MoveVectorState::Invalid);
const MoveVector MoveVector::TargetReached(glm::vec3(), 0.0f, MoveVectorState::TargetReached);

MoveVector::MoveVector(const glm::vec3& vec3, float rotation, MoveVectorState state) :
		_vec3(vec3), _rotation(rotation), _state(state) {
	if (state == MoveVectorState::Valid) {
		glm_assert_vec3(_vec3);
	}
}

float MoveVector::getOrientation(float duration) const {
	const float pi2 = glm::two_pi<float>();
	const float rotation = _rotation + pi2;
	return glm::mod(rotation * duration, pi2);
}

}
