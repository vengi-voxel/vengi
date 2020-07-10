/**
 * @file
 */

#include "Axis.h"
#include "video/Camera.h"
#include "video/ScopedLineWidth.h"
#include "video/ScopedState.h"
#include "core/Color.h"
#include "core/GLM.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

namespace render {

Axis::Axis(bool flipZ) : _flipZ(flipZ) {
	setPosition(glm::vec3(0));
}

void Axis::render(const video::Camera& camera, uint32_t renderMask) {
	video::ScopedState disableDepthTest(video::State::DepthTest, false);

	const glm::mat4& t = glm::translate(_pos);
	{
		video::ScopedLineWidth width(_lineWidth);
		const glm::mat4& model = glm::scale(t, _size);
		if ((renderMask & AxisMask::RenderXAxis) != 0u) {
			_shapeRenderer.render(_meshXIndex, camera, model);
		}
		if ((renderMask & AxisMask::RenderYAxis) != 0u) {
			_shapeRenderer.render(_meshYIndex, camera, model);
		}
		if ((renderMask & AxisMask::RenderZAxis) != 0u) {
			_shapeRenderer.render(_meshZIndex, camera, model);
		}
	}
	if (glm::abs(_size.x) > 1.0f && (renderMask & AxisMask::RenderXCone) != 0u) {
		_shapeRenderer.render(_coneXIndex, camera, t);
	}
	if (glm::abs(_size.y) > 1.0f && (renderMask & AxisMask::RenderYCone) != 0u) {
		_shapeRenderer.render(_coneYIndex, camera, t);
	}
	if (glm::abs(_size.z) > 1.0f && (renderMask & AxisMask::RenderZCone) != 0u) {
		_shapeRenderer.render(_coneZIndex, camera, t);
	}
}

void Axis::shutdown() {
	_shapeRenderer.shutdown();
	_shapeBuilder.shutdown();
	_meshXIndex = -1;
	_meshYIndex = -1;
	_meshZIndex = -1;
	_coneXIndex = -1;
	_coneYIndex = -1;
	_coneZIndex = -1;
}

void Axis::setSize(float x, float y, float z) {
	const glm::vec3 oldSize = _size;
	_size = glm::vec3(x, y, z);
	if (_meshXIndex != -1 && oldSize != _size) {
		createCones();
	}
}

bool Axis::init() {
	if (!_shapeRenderer.init()) {
		return false;
	}

	_shapeBuilder.setColor(core::Color::Red);
	_shapeBuilder.line(glm::zero<glm::vec3>(), glm::right);
	_meshXIndex = _shapeRenderer.create(_shapeBuilder);

	_shapeBuilder.clear();
	_shapeBuilder.setColor(core::Color::Green);
	_shapeBuilder.line(glm::zero<glm::vec3>(), glm::up);
	_meshYIndex = _shapeRenderer.create(_shapeBuilder);

	_shapeBuilder.clear();
	_shapeBuilder.setColor(core::Color::Blue);
	_shapeBuilder.line(glm::zero<glm::vec3>(), _flipZ ? glm::backward : glm::forward);
	_meshZIndex = _shapeRenderer.create(_shapeBuilder);

	createCones();
	return true;
}

void Axis::createCones() {
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
