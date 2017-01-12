#pragma once

#include "GLTypes.h"
#include <glm/vec4.hpp>

namespace video {

inline void clearColor(const glm::vec4& clearColor) {
	glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
}

inline void clear(ClearFlag flag) {
	glClear(std::enum_value(flag));
}

}
