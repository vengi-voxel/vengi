/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include "Types.h"

namespace video {

class ScopedPolygonMode {
private:
	GLint _polygonOffsetMode = GL_NONE;
public:
	inline ScopedPolygonMode(const video::PolygonMode mode) {
		glPolygonMode(GL_FRONT_AND_BACK, std::enum_value(mode));
		GL_checkError();
	}

	inline ScopedPolygonMode(const video::PolygonMode mode, const glm::vec2& offset) :
			ScopedPolygonMode(mode) {
		if (mode == video::PolygonMode::Points) {
			_polygonOffsetMode = GL_POLYGON_OFFSET_POINT;
		} else if (mode == video::PolygonMode::WireFrame) {
			_polygonOffsetMode = GL_POLYGON_OFFSET_LINE;
		} else if (mode == video::PolygonMode::Solid) {
			_polygonOffsetMode = GL_POLYGON_OFFSET_FILL;
		}
		if (_polygonOffsetMode != GL_NONE) {
			glEnable(_polygonOffsetMode);
			GL_checkError();
			glPolygonOffset(offset.x, offset.y);
			GL_checkError();
		}
	}

	inline ~ScopedPolygonMode() {
		if (_polygonOffsetMode != GL_NONE) {
			glDisable(_polygonOffsetMode);
			GL_checkError();
		}

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		GL_checkError();
	}
};

}
