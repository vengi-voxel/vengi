/**
 * @file
 */

#include "Axis.h"
#include "video/Camera.h"
#include "video/ScopedState.h"
#include "color/Color.h"
#include "math/Mat4f.h"
#include "math/Vec3f.h"
#include "math/Axes.h"
#include "math/Scalar.h"

namespace render {

Axis::Axis(bool flipZ) : _flipZ(flipZ) {
	setPosition(math::Vec3f(0));
}

void Axis::render(const video::Camera& camera, uint32_t renderMask) {
	video::ScopedState disableDepthTest(video::State::DepthTest, false);

	const math::Mat4f& t = math::translate(_pos);
	_shapeRenderer.setLineWidth(_lineWidth);
	const math::Mat4f& model = math::scale(t, _size);
	if ((renderMask & AxisMask::RenderXAxis) != 0u) {
		_shapeRenderer.render(_meshXIndex, camera, model);
	}
	if ((renderMask & AxisMask::RenderYAxis) != 0u) {
		_shapeRenderer.render(_meshYIndex, camera, model);
	}
	if ((renderMask & AxisMask::RenderZAxis) != 0u) {
		_shapeRenderer.render(_meshZIndex, camera, model);
	}
	if (math::abs(_size.x) > 1.0f && (renderMask & AxisMask::RenderXCone) != 0u) {
		_shapeRenderer.render(_coneXIndex, camera, t);
	}
	if (math::abs(_size.y) > 1.0f && (renderMask & AxisMask::RenderYCone) != 0u) {
		_shapeRenderer.render(_coneYIndex, camera, t);
	}
	if (math::abs(_size.z) > 1.0f && (renderMask & AxisMask::RenderZCone) != 0u) {
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
	const math::Vec3f oldSize = _size;
	_size = math::Vec3f(x, y, z);
	if (_meshXIndex != -1 && oldSize != _size) {
		createCones();
	}
}

bool Axis::init() {
	if (!_shapeRenderer.init()) {
		return false;
	}

	_shapeBuilder.setColor(color::Red());
	_shapeBuilder.line(math::Vec3f(0.0f), math::right());
	_meshXIndex = _shapeRenderer.create(_shapeBuilder);

	_shapeBuilder.clear();
	_shapeBuilder.setColor(color::Green());
	_shapeBuilder.line(math::Vec3f(0.0f), math::up());
	_meshYIndex = _shapeRenderer.create(_shapeBuilder);

	_shapeBuilder.clear();
	_shapeBuilder.setColor(color::Blue());
	_shapeBuilder.line(math::Vec3f(0.0f), _flipZ ? math::backward() : math::forward());
	_meshZIndex = _shapeRenderer.create(_shapeBuilder);

	createCones();
	return true;
}

void Axis::createCones() {
	_shapeBuilder.clear();
	_shapeBuilder.setColor(color::Red());
	_shapeBuilder.setRotation(math::rotate(math::threeOverTwoPi, math::Vec3f(0.0f, 1.0f, 0.0f)));
	_shapeBuilder.setPosition(math::Vec3f(_pos.x + _size.x, _pos.y, _pos.z));
	_shapeBuilder.cone(0.3f, 1.0f);
	_shapeRenderer.createOrUpdate(_coneXIndex, _shapeBuilder);

	_shapeBuilder.clear();
	_shapeBuilder.setColor(color::Green());
	_shapeBuilder.setRotation(math::rotate(math::halfPi, math::Vec3f( 1.0f,  0.0f,  0.0f)));
	_shapeBuilder.setPosition(math::Vec3f(_pos.x, _pos.y + _size.y, _pos.z));
	_shapeBuilder.cone(0.3f, 1.0f);
	_shapeRenderer.createOrUpdate(_coneYIndex, _shapeBuilder);

	_shapeBuilder.clear();
	_shapeBuilder.setColor(color::Blue());
	_shapeBuilder.setRotation(math::rotate(math::pi, math::Vec3f(0.0f, 1.0f, 0.0f)));
	_shapeBuilder.setPosition(math::Vec3f(_pos.x, _pos.y, _pos.z + _size.z));
	_shapeBuilder.cone(0.3f, 1.0f);
	_shapeRenderer.createOrUpdate(_coneZIndex, _shapeBuilder);
}

void Axis::setPosition(const math::Vec3f& pos) {
	_pos = pos;
}

}
