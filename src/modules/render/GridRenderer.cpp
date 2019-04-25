/**
 * @file
 */

#include "GridRenderer.h"
#include "core/Trace.h"
#include "math/AABB.h"
#include "math/Plane.h"

namespace render {

GridRenderer::GridRenderer(bool renderAABB, bool renderGrid) :
		_renderAABB(renderAABB), _renderGrid(renderGrid) {
}

bool GridRenderer::init() {
	if (!_shapeRenderer.init()) {
		Log::error("Failed to initialize the shape renderer");
		return false;
	}

	return true;
}

bool GridRenderer::setGridResolution(int resolution) {
	if (resolution < 1) {
		return false;
	}
	_resolution = resolution;
	_dirty = true;
	return true;
}

int GridRenderer::gridResolution() const {
	return _resolution;
}

void GridRenderer::update(const voxel::Region& region) {
	const math::AABB<int>& intaabb = region.aabb();
	const math::AABB<float> aabb(glm::vec3(intaabb.getLowerCorner()), glm::vec3(intaabb.getUpperCorner()));
	_shapeBuilder.clear();
	_shapeBuilder.aabb(aabb, false);
	_shapeRenderer.createOrUpdate(_aabbMeshIndex, _shapeBuilder);

	_shapeBuilder.clear();
	_shapeBuilder.aabbGridXY(aabb, false, (float)_resolution);
	_shapeRenderer.createOrUpdate(_gridMeshIndexXYFar, _shapeBuilder);

	_shapeBuilder.clear();
	_shapeBuilder.aabbGridXZ(aabb, false, (float)_resolution);
	_shapeRenderer.createOrUpdate(_gridMeshIndexXZFar, _shapeBuilder);

	_shapeBuilder.clear();
	_shapeBuilder.aabbGridYZ(aabb, false, (float)_resolution);
	_shapeRenderer.createOrUpdate(_gridMeshIndexYZFar, _shapeBuilder);

	_shapeBuilder.clear();
	_shapeBuilder.aabbGridXY(aabb, true, (float)_resolution);
	_shapeRenderer.createOrUpdate(_gridMeshIndexXYNear, _shapeBuilder);

	_shapeBuilder.clear();
	_shapeBuilder.aabbGridXZ(aabb, true, (float)_resolution);
	_shapeRenderer.createOrUpdate(_gridMeshIndexXZNear, _shapeBuilder);

	_shapeBuilder.clear();
	_shapeBuilder.aabbGridYZ(aabb, true, (float)_resolution);
	_shapeRenderer.createOrUpdate(_gridMeshIndexYZNear, _shapeBuilder);

	_dirty = false;
}

void GridRenderer::clear() {
	 _shapeBuilder.clear();
	 _dirty = false;
}

void GridRenderer::render(const video::Camera& camera, const voxel::Region& region) {
	core_trace_scoped(GridRendererRender);

	if (_dirty) {
		update(region);
	}

	_shapeRenderer.hide(_aabbMeshIndex, !_renderAABB);
	if (_renderGrid) {
		const glm::vec3 center(region.getCentre());
		const glm::vec3& halfWidth = glm::vec3(region.getDimensionsInCells()) / 2.0f;
		const math::Plane planeLeft  (glm::left,     center + glm::vec3(-halfWidth.x, 0.0f, 0.0f));
		const math::Plane planeRight (glm::right,    center + glm::vec3( halfWidth.x, 0.0f, 0.0f));
		const math::Plane planeBottom(glm::down,     center + glm::vec3(0.0f, -halfWidth.y, 0.0f));
		const math::Plane planeTop   (glm::up,       center + glm::vec3(0.0f,  halfWidth.y, 0.0f));
		const math::Plane planeNear  (glm::forward,  center + glm::vec3(0.0f, 0.0f, -halfWidth.z));
		const math::Plane planeFar   (glm::backward, center + glm::vec3(0.0f, 0.0f,  halfWidth.z));

		_shapeRenderer.hide(_gridMeshIndexXYFar,  !planeFar.isBackSide(camera.position()));
		_shapeRenderer.hide(_gridMeshIndexXYNear, !planeNear.isBackSide(camera.position()));
		_shapeRenderer.hide(_gridMeshIndexXZFar,  !planeTop.isBackSide(camera.position()));
		_shapeRenderer.hide(_gridMeshIndexXZNear, !planeBottom.isBackSide(camera.position()));
		_shapeRenderer.hide(_gridMeshIndexYZFar,  !planeRight.isBackSide(camera.position()));
		_shapeRenderer.hide(_gridMeshIndexYZNear, !planeLeft.isBackSide(camera.position()));
	} else {
		_shapeRenderer.hide(_gridMeshIndexXYFar,  true);
		_shapeRenderer.hide(_gridMeshIndexXYNear, true);
		_shapeRenderer.hide(_gridMeshIndexXZFar,  true);
		_shapeRenderer.hide(_gridMeshIndexXZNear, true);
		_shapeRenderer.hide(_gridMeshIndexYZFar,  true);
		_shapeRenderer.hide(_gridMeshIndexYZNear, true);
	}
	_shapeRenderer.renderAll(camera);
}

void GridRenderer::shutdown() {
	_aabbMeshIndex = -1;
	_gridMeshIndexXYNear = -1;
	_gridMeshIndexXYFar = -1;
	_gridMeshIndexXZNear = -1;
	_gridMeshIndexXZFar = -1;
	_gridMeshIndexYZNear = -1;
	_gridMeshIndexYZFar = -1;
	_shapeRenderer.shutdown();
	_shapeBuilder.shutdown();
}

}
