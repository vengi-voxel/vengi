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

inline bool enable(State state) {
	if (state == State::DepthMask) {
		glDepthMask(GL_TRUE);
	} else {
		glEnable(std::enum_value(state));
	}
	return true;
}

inline bool disable(State state) {
	if (state == State::DepthMask) {
		glDepthMask(GL_TRUE);
	} else {
		glDisable(std::enum_value(state));
	}
	return true;
}

inline void cullFace(Face face) {
	glCullFace(std::enum_value(face));
}

inline bool depthFunc(CompareFunc func) {
	glDepthFunc(std::enum_value(func));
	return true;
}

inline bool blendFunc(BlendMode src, BlendMode dest) {
	glBlendFunc(std::enum_value(src), std::enum_value(dest));
	return true;
}

}
