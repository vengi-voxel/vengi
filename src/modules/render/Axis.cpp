/**
 * @file
 */

#include "Axis.h"
#include "video/ScopedLineWidth.h"
#include "video/ScopedState.h"

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
	_meshIndex = -1;
}

void Axis::setSize(float x, float y, float z) {
	_shapeBuilder.clear();
	_shapeBuilder.axis(glm::vec3(x, y, z));
	_shapeRenderer.createOrUpdate(_meshIndex, _shapeBuilder);
}

bool Axis::init() {
	if (!_shapeRenderer.init()) {
		return false;
	}
	_shapeBuilder.axis(20.0f);
	_meshIndex = _shapeRenderer.create(_shapeBuilder);
	return _meshIndex >= 0;
}

void Axis::setPosition(const glm::vec3& pos) {
	_pos = pos;
}

}
