/**
 * @file
 */

#include "Plane.h"

namespace frontend {

void Plane::render(const video::Camera& camera, video::Shader* shader) {
	render(camera, glm::mat4(1.f), shader);
}

void Plane::render(const video::Camera& camera, const glm::mat4& model, video::Shader* shader) {
	const bool disabled = video::disable(video::State::CullFace);
	_shapeRenderer.render(_planeMesh, camera, model, shader);
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

bool Plane::plane(const glm::vec3& position, int tesselation, float scale, const glm::vec4& color) {
	_shapeBuilder.setColor(color);
	_shapeBuilder.setPosition(position);
	_shapeBuilder.plane(tesselation, scale);
	_shapeRenderer.createOrUpdate(_planeMesh, _shapeBuilder);
	return _planeMesh >= 0;
}

bool Plane::plane(const glm::vec3& position, const math::Plane& plane, const glm::vec4& color) {
	_shapeBuilder.setColor(color);
	_shapeBuilder.setPosition(position);
	_shapeBuilder.plane(plane, false);
	_shapeRenderer.createOrUpdate(_planeMesh, _shapeBuilder);
	return _planeMesh >= 0;
}

}
