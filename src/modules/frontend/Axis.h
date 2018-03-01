/**
 * @file
 */

#pragma once

#include "video/Camera.h"
#include "video/ShapeBuilder.h"
#include "frontend/ShapeRenderer.h"
#include "video/ScopedLineWidth.h"

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
		const bool active = video::disable(video::State::DepthTest);
		video::ScopedLineWidth width(_lineWidth);
		_shapeRenderer.renderAll(camera);
		if (active) {
			video::enable(video::State::DepthTest);
		}
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
		return _shapeRenderer.create(_shapeBuilder) >= 0;
	};
};

}
