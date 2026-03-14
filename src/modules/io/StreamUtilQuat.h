/**
 * @file
 * @brief Quaternion stream utilities - separated from StreamUtil.h to avoid pulling in quaternion.hpp everywhere
 */

#pragma once

#include "io/Stream.h"
#include <glm/gtc/quaternion.hpp>

namespace io {

inline bool readQuat(io::ReadStream &s, glm::quat &q) {
	return s.readFloat(q.x) == 0 && s.readFloat(q.y) == 0 && s.readFloat(q.z) == 0 && s.readFloat(q.w) == 0;
}

} // namespace io
