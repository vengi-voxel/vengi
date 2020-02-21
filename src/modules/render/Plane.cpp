/**
 * @file
 */

#include "Plane.h"
#include "core/ArrayLength.h"

namespace render {

void Plane::render(const video::Camera& camera, const glm::mat4& model) const {
	const bool disabled = video::disable(video::State::CullFace);
	_shapeRenderer.renderAll(camera, model);
	if (disabled) {
		video::enable(video::State::CullFace);
	}
}

void Plane::shutdown() {
	_shapeRenderer.shutdown();
	_shapeBuilder.shutdown();
}

bool Plane::init() {
	if (!_shapeRenderer.init()) {
		return false;
	}
	return true;
}

void Plane::clear() {
	for (int32_t i = 0; i < lengthof(_planeMeshes); ++i) {
		if (!_planeMeshes[i]) {
			continue;
		}
		_shapeRenderer.deleteMesh(i);
		_planeMeshes[i] = false;
	}
}

bool Plane::plane(const glm::vec3& position, int tesselation, const glm::vec4& color) {
	_shapeBuilder.setColor(color);
	_shapeBuilder.setPosition(position);
	_shapeBuilder.plane(tesselation);
	const int32_t id = _shapeRenderer.create(_shapeBuilder);
	if (id >= 0 && id < lengthof(_planeMeshes)) {
		_planeMeshes[id] = true;
	}
	return id >= 0;
}

bool Plane::plane(const glm::vec3& position, const math::Plane& plane, const glm::vec4& color) {
	_shapeBuilder.setColor(color);
	_shapeBuilder.setPosition(position);
	_shapeBuilder.plane(plane, false);
	const int32_t id = _shapeRenderer.create(_shapeBuilder);
	if (id >= 0 && id < lengthof(_planeMeshes)) {
		_planeMeshes[id] = true;
	}
	return id >= 0;
}

}
