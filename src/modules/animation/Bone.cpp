/**
 * @file
 */

#include "Bone.h"
#include "core/GLM.h"
#include "core/Assert.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/functions.hpp>

namespace animation {

glm::mat4 Bone::matrix() const {
	return glm::translate(translation) * glm::mat4_cast(orientation) * glm::scale(scale);
}

void Bone::lerp(const Bone& previous, double deltaFrameSeconds) {
	const float factor = glm::clamp((float)(deltaFrameSeconds * 16.0), 0.0f, 1.0f);
	translation = glm::mix(previous.translation, translation, factor);
	orientation = glm::normalize(glm::slerp(previous.orientation, orientation, factor));
	scale = glm::mix(previous.scale, scale, factor);
}

}
