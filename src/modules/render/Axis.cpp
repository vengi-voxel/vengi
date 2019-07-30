/**
 * @file
 */

#include "Axis.h"
#include <glm/gtx/intersect.hpp>

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

void Axis::update(const video::Camera& camera, const glm::vec2& pixelPos) {
	static glm::vec2 lastPixelPos = pixelPos;
	if (lastPixelPos == pixelPos) {
		return;
	}
	lastPixelPos = pixelPos;
	const video::Ray& ray = camera.mouseRay(pixelPos);

	const glm::vec3 p1 = ray.origin;
	const glm::vec3 p2 = p1 + ray.direction * camera.farPlane();

	glm::vec3 paX;
	glm::vec3 pbX;
	glm::vec3 paY;
	glm::vec3 pbY;
	glm::vec3 paZ;
	glm::vec3 pbZ;

	const bool intersectX = glm::intersectLines(p1, p2, glm::zero<glm::vec3>(), glm::vec3(20.0f,  0.0f,  0.0f), paX, pbX);
	const bool intersectY = glm::intersectLines(p1, p2, glm::zero<glm::vec3>(), glm::vec3( 0.0f, 20.0f,  0.0f), paY, pbY);
	const bool intersectZ = glm::intersectLines(p1, p2, glm::zero<glm::vec3>(), glm::vec3( 0.0f,  0.0f, 20.0f), paZ, pbZ);
	const float distanceX = intersectX ? glm::distance2(paX, pbX) : FLT_MAX;
	const float distanceY = intersectY ? glm::distance2(paY, pbY) : FLT_MAX;
	const float distanceZ = intersectZ ? glm::distance2(paZ, pbZ) : FLT_MAX;

	const float distanceToLine = 0.2f;
	if (distanceX < distanceY && distanceX < distanceZ && distanceX < distanceToLine) {
		_mode = Mode::TranslateX;
		_shapeBuilder.clear();
		_shapeBuilder.axis(glm::vec3(40.0f, 20.0f, 20.0f));
		_shapeRenderer.createOrUpdate(_meshIndex, _shapeBuilder);
	} else if (distanceY < distanceX && distanceY < distanceZ && distanceY < distanceToLine) {
		_mode = Mode::TranslateY;
		_shapeBuilder.clear();
		_shapeBuilder.axis(glm::vec3(20.0f, 40.0f, 20.0f));
		_shapeRenderer.createOrUpdate(_meshIndex, _shapeBuilder);
	} else if (distanceZ < distanceX && distanceZ < distanceY && distanceZ < distanceToLine) {
		_mode = Mode::TranslateZ;
		_shapeBuilder.clear();
		_shapeBuilder.axis(glm::vec3(20.0f, 20.0f, 40.0f));
		_shapeRenderer.createOrUpdate(_meshIndex, _shapeBuilder);
	} else {
		_mode = Mode::None;
		_shapeBuilder.clear();
		_shapeBuilder.axis(glm::vec3(20.0f, 20.0f, 20.0f));
		_shapeRenderer.createOrUpdate(_meshIndex, _shapeBuilder);
	}
}

}
