/**
 * @file
 */

#include "ICharacter.h"
#include "core/GLM.h"

namespace backend {

void ICharacter::setPosition(const glm::vec3& position) {
	glm_assert_vec3(position);
	_position = position;
}

}
