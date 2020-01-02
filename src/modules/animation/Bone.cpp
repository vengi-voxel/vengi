/**
 * @file
 */

#include "Bone.h"
#include "core/GLM.h"

namespace animation {

glm::mat4 Bone::matrix() const {
	return glm::translate(translation) * glm::mat4_cast(orientation) * glm::scale(scale);
}

void Bone::lerp(const Bone& previous, float dt) {
	translation += (translation - previous.translation) * dt;
	orientation = glm::normalize(glm::slerp(previous.orientation, orientation, dt));
	scale += (scale - previous.scale) * dt;
}

}
