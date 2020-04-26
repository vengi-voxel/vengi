/**
 * @file
 */

#include "Bone.h"
#include "core/GLM.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/functions.hpp>

namespace animation {

glm::mat4 Bone::matrix() const {
	return glm::translate(translation) * glm::mat4_cast(orientation) * glm::scale(scale);
}

void Bone::lerp(const Bone& previous, float dt) {
	translation = glm::mix(previous.translation, translation, dt);
	orientation = glm::normalize(glm::slerp(previous.orientation, orientation, dt));
	scale = glm::mix(previous.scale, scale, dt);
}

}
