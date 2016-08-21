#pragma once

#include "video/Camera.h"
#include "video/ShapeBuilder.h"
#include "frontend/ShapeRenderer.h"

namespace frontend {

class Axis {
private:
	video::ShapeBuilder _shapeBuilder;
	frontend::ShapeRenderer _shapeRenderer;
public:
	void render(const video::Camera& camera) {
		glDisable(GL_DEPTH_TEST);
		glLineWidth(4.0f);
		_shapeRenderer.renderAll(camera, GL_LINES);
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
		return _shapeRenderer.createMesh(_shapeBuilder) >= 0;
	};
};

}
