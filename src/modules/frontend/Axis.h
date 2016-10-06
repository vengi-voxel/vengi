#pragma once

#include "video/Camera.h"
#include "video/ShapeBuilder.h"
#include "frontend/ShapeRenderer.h"

namespace frontend {

/**
 * @brief Renders a world axis (xyz)
 *
 * @see video::ShapeBuilder
 * @see ShapeRenderer
 */
class Axis {
private:
	video::ShapeBuilder _shapeBuilder;
	frontend::ShapeRenderer _shapeRenderer;
	float _lineWidth = 4.0f;
public:
	void render(const video::Camera& camera) {
		glDisable(GL_DEPTH_TEST);
		glLineWidth(_lineWidth);
		_shapeRenderer.renderAll(camera);
		glLineWidth(1.0f);
		glEnable(GL_DEPTH_TEST);
	}

	void shutdown() {
		_shapeRenderer.shutdown();
		_shapeBuilder.shutdown();
	}

	bool init() {
		if (!_shapeRenderer.init()) {
			return false;
		}
		_shapeBuilder.axis(20.0f);
		GLdouble buf[2];
		glGetDoublev(GL_SMOOTH_LINE_WIDTH_RANGE, buf);
		_lineWidth = std::min((float)buf[1], _lineWidth);
		return _shapeRenderer.createMesh(_shapeBuilder) >= 0;
	};
};

}
