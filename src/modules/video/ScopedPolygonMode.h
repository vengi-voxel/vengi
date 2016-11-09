/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include "Types.h"

namespace video {

class ScopedPolygonMode {
private:
	GLint _polygonMode[2] = {GL_NONE, GL_NONE};
	GLint _polygonOffsetMode = GL_NONE;
public:
	inline ScopedPolygonMode(const video::PolygonMode mode) {
		if (mode == video::PolygonMode::Solid) {
			return;
		}
		glGetIntegerv(GL_POLYGON_MODE, _polygonMode);
		glPolygonMode(GL_FRONT_AND_BACK, std::enum_value(mode));
		GL_checkError();
	}

	inline ScopedPolygonMode(const video::PolygonMode mode, const glm::vec2& offset) {
		if (mode == video::PolygonMode::Solid) {
			return;
		}
		glGetIntegerv(GL_POLYGON_MODE, _polygonMode);
		glPolygonMode(GL_FRONT_AND_BACK, std::enum_value(mode));
		if (mode == video::PolygonMode::Points) {
			_polygonOffsetMode = GL_POLYGON_OFFSET_POINT;
		} else if (mode == video::PolygonMode::WireFrame) {
			_polygonOffsetMode = GL_POLYGON_OFFSET_LINE;
		} else if (mode == video::PolygonMode::Solid) {
			_polygonOffsetMode = GL_POLYGON_OFFSET_FILL;
		}
		if (_polygonOffsetMode != GL_NONE) {
			glEnable(_polygonOffsetMode);
			glPolygonOffset(offset.x, offset.y);
		}
		GL_checkError();
	}

	inline ~ScopedPolygonMode() {
		if (_polygonMode[0] == GL_NONE) {
			return;
		}

		if (_polygonOffsetMode != GL_NONE) {
			glDisable(_polygonOffsetMode);
			GL_checkError();
		}

		if (_polygonMode[0] == _polygonMode[1]) {
			glPolygonMode(GL_FRONT_AND_BACK, _polygonMode[0]);
			GL_checkError();
		} else {
			glPolygonMode(GL_FRONT, _polygonMode[0]);
			glPolygonMode(GL_BACK, _polygonMode[1]);
			GL_checkError();
		}
	}
};

}
