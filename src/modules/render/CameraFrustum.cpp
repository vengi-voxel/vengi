/**
 * @file
 */

#include "CameraFrustum.h"
#include "color/ColorUtil.h"
#include "core/Trace.h"
#include "video/Renderer.h"
#include "math/AABB.h"
#include "video/Camera.h"

namespace render {

bool CameraFrustum::init(const glm::vec4& color, int splitFrustum) {
	_splitFrustum = splitFrustum;
	_color = color;
	if (!_shapeRenderer.init()) {
		return false;
	}
	_shapeBuilder.setColor(_color);
	_shapeBuilder.cube(glm::vec3(0), glm::vec3(1));
	_frustumMesh = _shapeRenderer.create(_shapeBuilder);
	if (_frustumMesh == -1) {
		return false;
	}
	return true;
}

void CameraFrustum::shutdown() {
	_shapeRenderer.shutdown();
	_shapeBuilder.shutdown();
}

void CameraFrustum::render(const video::Camera& camera, const video::Camera& frustumCamera) {
	core_trace_scoped(CameraFrustumRender);
	_shapeBuilder.clear();
	_shapeBuilder.setColor(_color);
	_shapeBuilder.frustum(frustumCamera, _splitFrustum);
	_shapeRenderer.update(_frustumMesh, _shapeBuilder);
	if (_renderAABB) {
		const math::AABB<float>& aabb = frustumCamera.aabb();
		_shapeBuilder.clear();
		_shapeBuilder.setColor(color::brighter(_color));
		_shapeBuilder.aabb(aabb);
		_shapeRenderer.createOrUpdate(_aabbMesh, _shapeBuilder);
	} else if (_aabbMesh >= 0) {
		_shapeRenderer.deleteMesh(_aabbMesh);
		_aabbMesh = -1;
	}
	_shapeRenderer.renderAll(camera);
}

}
