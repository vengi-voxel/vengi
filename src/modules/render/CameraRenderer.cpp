/**
 * @file
 */

#include "CameraRenderer.h"
#include "core/TraceScoped.h"
#include "color/ColorUtil.h"
#include "math/AABB.h"
#include "video/Camera.h"
#include "video/Renderer.h"
#include "math/Vec4f.h"
#include "math/Vec3f.h"

namespace render {

bool CameraRenderer::init(int splitFrustum) {
	_splitFrustum = splitFrustum;
	if (!_shapeRenderer.init()) {
		return false;
	}
	_shapeBuilder.cube(math::Vec3f(0), math::Vec3f(1));
	_frustumMesh = _shapeRenderer.create(_shapeBuilder);
	if (_frustumMesh == -1) {
		return false;
	}
	return true;
}

void CameraRenderer::shutdown() {
	_shapeRenderer.shutdown();
	_shapeBuilder.shutdown();
}

void CameraRenderer::render(const video::Camera &camera, const Node &frustumCamera) {
	core_trace_scoped(CameraRender);
	_shapeBuilder.clear();
	const math::Vec4f color(color::fromRGBA(frustumCamera.color));
	_shapeBuilder.setColor(color);
	if (_splitFrustum > 0) {
		_shapeBuilder.frustum(frustumCamera.camera, _splitFrustum);
	} else {
		_shapeBuilder.camera(frustumCamera.camera);
	}
	_shapeRenderer.update(_frustumMesh, _shapeBuilder);
	if (_renderAABB) {
		const math::AABB<float> &aabb = frustumCamera.camera.aabb();
		_shapeBuilder.clear();
		_shapeBuilder.setColor(color::brighter(color));
		_shapeBuilder.aabb(aabb);
		_shapeRenderer.createOrUpdate(_aabbMesh, _shapeBuilder);
	} else if (_aabbMesh >= 0) {
		_shapeRenderer.deleteMesh(_aabbMesh);
		_aabbMesh = -1;
	}
	_shapeRenderer.renderAll(camera);
}

} // namespace render
