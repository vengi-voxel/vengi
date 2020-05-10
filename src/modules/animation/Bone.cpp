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
	const float factor = (float)(deltaFrameSeconds * 16.0);
	core_assert(deltaFrameSeconds >= 0.0 && deltaFrameSeconds < 1.0);
	translation = glm::mix(previous.translation, translation, factor);
	orientation = glm::normalize(glm::slerp(previous.orientation, orientation, factor));
	scale = glm::mix(previous.scale, scale, factor);
}

}
