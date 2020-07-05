/**
 * @file
 */

#include "Axis.h"
#include "video/Camera.h"
#include "video/ScopedLineWidth.h"
#include "video/ScopedState.h"
#include "core/Color.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

namespace render {

Axis::Axis(bool flipZ, bool arrowHeads) : _flipZ(flipZ), _arrowHeads(arrowHeads) {
	setPosition(glm::vec3(0));
}

void Axis::render(const video::Camera& camera) {
	video::ScopedState disableDepthTest(video::State::DepthTest, false);

	const glm::mat4& t = glm::translate(_pos);
	{
		video::ScopedLineWidth width(_lineWidth);
		const glm::mat4& model = glm::scale(t, _size);
		_shapeRenderer.render(_meshIndex, camera, model);
	}
	if (_size.x > 1.0f) {
		_shapeRenderer.render(_coneXIndex, camera, t);
	}
	if (_size.y > 1.0f) {
		_shapeRenderer.render(_coneYIndex, camera, t);
	}
	if (_size.z > 1.0f) {
		_shapeRenderer.render(_coneZIndex, camera, t);
	}
}

void Axis::shutdown() {
	_shapeRenderer.shutdown();
	_shapeBuilder.shutdown();
	_meshIndex = -1;
	_coneXIndex = -1;
	_coneYIndex = -1;
	_coneZIndex = -1;
}

void Axis::setSize(float x, float y, float z) {
	const glm::vec3 oldSize = _size;
	_size = glm::vec3(x, y, z);
	if (_meshIndex != -1 && oldSize != _size) {
		createCones();
	}
}

bool Axis::init() {
	if (!_shapeRenderer.init()) {
		return false;
	}
	_shapeBuilder.axis(glm::vec3(1.0f, 1.0f, _flipZ ? -1.0f : 1.0f));
	_meshIndex = _shapeRenderer.create(_shapeBuilder);

	createCones();
	return true;
}

void Axis::createCones() {
	if (!_arrowHeads) {
		return;
	}
	_shapeBuilder.clear();
	_shapeBuilder.setColor(core::Color::Red);
	_shapeBuilder.setRotation(glm::rotate(glm::three_over_two_pi<float>(), glm::vec3(0.0f, 1.0f, 0.0f)));
	_shapeBuilder.setPosition(glm::vec3(_pos.x + _size.x, _pos.y, _pos.z));
	_shapeBuilder.cone(0.3f, 1.0f);
	_shapeRenderer.createOrUpdate(_coneXIndex, _shapeBuilder);

	_shapeBuilder.clear();
	_shapeBuilder.setColor(core::Color::Green);
	_shapeBuilder.setRotation(glm::rotate(glm::half_pi<float>(), glm::vec3( 1.0f,  0.0f,  0.0f)));
	_shapeBuilder.setPosition(glm::vec3(_pos.x, _pos.y + _size.y, _pos.z));
	_shapeBuilder.cone(0.3f, 1.0f);
	_shapeRenderer.createOrUpdate(_coneYIndex, _shapeBuilder);

	_shapeBuilder.clear();
	_shapeBuilder.setColor(core::Color::Blue);
	_shapeBuilder.setRotation(glm::rotate(glm::pi<float>(), glm::vec3(0.0f, 1.0f, 0.0f)));
	_shapeBuilder.setPosition(glm::vec3(_pos.x, _pos.y, _pos.z + _size.z));
	_shapeBuilder.cone(0.3f, 1.0f);
	_shapeRenderer.createOrUpdate(_coneZIndex, _shapeBuilder);
}

void Axis::setPosition(const glm::vec3& pos) {
	_pos = pos;
}

}
