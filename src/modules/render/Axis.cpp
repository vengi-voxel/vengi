/**
 * @file
 */

#include "Axis.h"
#include "video/Camera.h"
#include "video/ScopedLineWidth.h"
#include "video/ScopedState.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

namespace render {

Axis::Axis(bool flipZ) : _flipZ(flipZ) {
	setPosition(glm::vec3(0));
}

void Axis::render(const video::Camera& camera) {
	video::ScopedState disableDepthTest(video::State::DepthTest, false);
	video::ScopedLineWidth width(_lineWidth);
	const glm::mat4& model = glm::translate(glm::scale(_size), _pos);
	_shapeRenderer.renderAll(camera, model);
}

void Axis::shutdown() {
	_shapeRenderer.shutdown();
	_shapeBuilder.shutdown();
	_meshIndex = -1;
}

void Axis::setSize(float x, float y, float z) {
	_size = glm::vec3(x, y, z);
}

bool Axis::init() {
	if (!_shapeRenderer.init()) {
		return false;
	}
	_shapeBuilder.axis(glm::vec3(1.0f, 1.0f, _flipZ ? -1.0f : 1.0f));
	_meshIndex = _shapeRenderer.create(_shapeBuilder);
	return _meshIndex >= 0;
}

void Axis::setPosition(const glm::vec3& pos) {
	_pos = pos;
}

}
