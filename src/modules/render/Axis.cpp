/**
 * @file
 */

#include "Axis.h"

namespace render {

Axis::Axis() {
	setPosition(glm::vec3(0));
}

void Axis::render(const video::Camera& camera) {
	video::ScopedState disableDepthTest(video::State::DepthTest, false);
	video::ScopedLineWidth width(_lineWidth);
	const glm::mat4& model = glm::translate(_pos);
	_shapeRenderer.renderAll(camera, model);
}

void Axis::shutdown() {
	_shapeRenderer.shutdown();
	_shapeBuilder.shutdown();
}

bool Axis::init() {
	if (!_shapeRenderer.init()) {
		return false;
	}
	_shapeBuilder.axis(20.0f);
	return _shapeRenderer.create(_shapeBuilder) >= 0;
}

void Axis::setPosition(const glm::vec3& pos) {
	_pos = pos;
}

}
