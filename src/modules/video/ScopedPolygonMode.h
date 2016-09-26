#pragma once

#include "core/Common.h"
#include "Types.h"

namespace video {

class ScopedPolygonMode {
private:
	GLint _polygonMode = GL_NONE;
public:
	inline ScopedPolygonMode(const video::PolygonMode mode) {
		if (mode == video::PolygonMode::Solid) {
			return;
		}
		glGetIntegerv(GL_POLYGON_MODE, &_polygonMode);
		glPolygonMode(GL_FRONT_AND_BACK, std::enum_value(mode));
	}

	inline ~ScopedPolygonMode() {
		if (_polygonMode == GL_NONE) {
			return;
		}
		glPolygonMode(GL_FRONT_AND_BACK, _polygonMode);
	}
};

}
