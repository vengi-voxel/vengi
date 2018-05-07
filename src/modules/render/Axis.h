/**
 * @file
 */

#pragma once

#include "video/Camera.h"
#include "video/ShapeBuilder.h"
#include "render/ShapeRenderer.h"
#include "video/ScopedLineWidth.h"
#include "video/ScopedState.h"
#include "core/IComponent.h"

namespace render {

/**
 * @brief Renders a world axis (xyz)
 *
 * @see video::ShapeBuilder
 * @see ShapeRenderer
 */
class Axis : public core::IComponent {
private:
	video::ShapeBuilder _shapeBuilder;
	render::ShapeRenderer _shapeRenderer;
	float _lineWidth = 4.0f;
public:
	void render(const video::Camera& camera) {
		video::ScopedState disableDepthTest(video::State::DepthTest, false);
		video::ScopedLineWidth width(_lineWidth);
		_shapeRenderer.renderAll(camera);
	}

	void shutdown() override {
		_shapeRenderer.shutdown();
		_shapeBuilder.shutdown();
	}

	bool init() override {
		if (!_shapeRenderer.init()) {
			return false;
		}
		_shapeBuilder.axis(20.0f);
		return _shapeRenderer.create(_shapeBuilder) >= 0;
	};
};

}
