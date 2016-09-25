#pragma once

#include "frontend/ShapeRenderer.h"
#include "video/ShapeBuilder.h"
#include "video/Camera.h"

class FrustumEntity {
private:
	video::ShapeBuilder _shapeBuilder;
	frontend::ShapeRenderer _shapeRenderer;

	int32_t _sphereMesh = 0;
	glm::vec4 _color;
	glm::vec3 _position;
	float _radius;
	video::FrustumResult _result = video::FrustumResult::Outside;

public:
	FrustumEntity(const glm::vec4& color = core::Color::Red, float radius = 5.0f) :
			_color(color), _radius(radius) {
	}

	FrustumEntity& setPosition(const glm::vec3& position) {
		_position = position;
		return *this;
	}

	FrustumEntity& setColor(const glm::vec4& color) {
		_color = color;
		return *this;
	}

	FrustumEntity& setRadius(float radius) {
		_radius = radius;
		return *this;
	}

	bool init() {
		if (!_shapeRenderer.init()) {
			return false;
		}

		_shapeBuilder.setPosition(_position);
		_shapeBuilder.setColor(_color);
		_shapeBuilder.sphere(10, 10, _radius);
		_sphereMesh = _shapeRenderer.createMesh(_shapeBuilder);

		return _sphereMesh != -1;
	}

	void shutdown() {
		_shapeRenderer.shutdown();
	}

	void cull(const video::Camera& cullCamera) {
		video::FrustumResult result = cullCamera.testFrustum(_position);
		if (_result == result) {
			return;
		}
		Log::debug("culling result changed to %i", std::enum_value(result));
		_result = result;
		glm::vec4 color = _color;
		switch (result) {
		case video::FrustumResult::Intersect:
			color = core::Color::Purple;
			break;
		case video::FrustumResult::Inside:
			color = core::Color::Green;
			break;
		case video::FrustumResult::Outside:
		default:
			break;
		}
		_shapeBuilder.clear();
		_shapeBuilder.setPosition(_position);
		_shapeBuilder.setColor(color);
		_shapeBuilder.sphere(10, 10, _radius);
		_shapeRenderer.update(_sphereMesh, _shapeBuilder);
	}

	void render(const video::Camera& camera) const {
		_shapeRenderer.renderAll(camera);
	}
};
