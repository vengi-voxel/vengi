/**
 * @file
 */

#pragma once

#include "core/GLM.h"

namespace math {

glm::ivec3 transform(const glm::mat4x4 &mat, const glm::ivec3 &pos, const glm::vec3 &pivot);
glm::vec3 transform(const glm::mat4x4 &mat, const glm::vec3 &pos, const glm::vec3 &pivot);

}
