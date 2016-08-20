#include "CameraFrustum.h"
#include "video/GLFunc.h"

namespace frontend {

bool CameraFrustum::init(const video::Camera& frustumCamera, const glm::vec4& color) {
	_shapeRenderer.init();
	_shapeBuilder.setColor(color);
	_shapeBuilder.frustum(frustumCamera);
	_frustumMesh = _shapeRenderer.createMesh(_shapeBuilder);
	return true;
}

void CameraFrustum::shutdown() {
	_shapeRenderer.shutdown();
	_shapeBuilder.shutdown();
}

void CameraFrustum::render(const video::Camera& camera, const video::Camera& frustumCamera) {
	_shapeBuilder.clear();
	_shapeBuilder.frustum(frustumCamera);
	_shapeRenderer.update(_frustumMesh, _shapeBuilder);
	if (_renderAABB) {
		const video::ShapeBuilder::Vertices& vertices = _shapeBuilder.getVertices();
		const glm::vec3& mins = vertices[1];
		const glm::vec3& maxs = vertices[6];
		const core::AABB<float> aabb(mins, maxs);
		_shapeBuilder.clear();
		_shapeBuilder.aabb(aabb);
		if (_aabbMesh < 0) {
			_aabbMesh = _shapeRenderer.createMesh(_shapeBuilder);
		} else {
			_shapeRenderer.update(_aabbMesh, _shapeBuilder);
		}
	} else if (_aabbMesh >= 0) {
		_shapeRenderer.deleteMesh(_aabbMesh);
		_aabbMesh = -1;
	}
	_shapeRenderer.renderAll(camera, GL_LINES);
}

}
