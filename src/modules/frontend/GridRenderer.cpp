#include "GridRenderer.h"

namespace frontend {

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

void GridRenderer::update(const voxel::Region& region) {
	const core::AABB<int>& intaabb = region.aabb();
	const core::AABB<float> aabb(glm::vec3(intaabb.getLowerCorner()), glm::vec3(intaabb.getUpperCorner()));
	_shapeBuilder.clear();
	_shapeBuilder.aabb(aabb, false);
	if (_aabbMeshIndex == -1) {
		_aabbMeshIndex = _shapeRenderer.createMesh(_shapeBuilder);
	} else {
		_shapeRenderer.update(_aabbMeshIndex, _shapeBuilder);
	}

	_shapeBuilder.clear();
	_shapeBuilder.aabbGridXY(aabb, false);
	if (_gridMeshIndexXYFar == -1) {
		_gridMeshIndexXYFar = _shapeRenderer.createMesh(_shapeBuilder);
	} else {
		_shapeRenderer.update(_gridMeshIndexXYFar, _shapeBuilder);
	}

	_shapeBuilder.clear();
	_shapeBuilder.aabbGridXZ(aabb, false);
	if (_gridMeshIndexXZFar == -1) {
		_gridMeshIndexXZFar = _shapeRenderer.createMesh(_shapeBuilder);
	} else {
		_shapeRenderer.update(_gridMeshIndexXZFar, _shapeBuilder);
	}

	_shapeBuilder.clear();
	_shapeBuilder.aabbGridYZ(aabb, false);
	if (_gridMeshIndexYZFar == -1) {
		_gridMeshIndexYZFar = _shapeRenderer.createMesh(_shapeBuilder);
	} else {
		_shapeRenderer.update(_gridMeshIndexYZFar, _shapeBuilder);
	}

	_shapeBuilder.clear();
	_shapeBuilder.aabbGridXY(aabb, true);
	if (_gridMeshIndexXYNear == -1) {
		_gridMeshIndexXYNear = _shapeRenderer.createMesh(_shapeBuilder);
	} else {
		_shapeRenderer.update(_gridMeshIndexXYNear, _shapeBuilder);
	}

	_shapeBuilder.clear();
	_shapeBuilder.aabbGridXZ(aabb, true);
	if (_gridMeshIndexXZNear == -1) {
		_gridMeshIndexXZNear = _shapeRenderer.createMesh(_shapeBuilder);
	} else {
		_shapeRenderer.update(_gridMeshIndexXZNear, _shapeBuilder);
	}

	_shapeBuilder.clear();
	_shapeBuilder.aabbGridYZ(aabb, true);
	if (_gridMeshIndexYZNear == -1) {
		_gridMeshIndexYZNear = _shapeRenderer.createMesh(_shapeBuilder);
	} else {
		_shapeRenderer.update(_gridMeshIndexYZNear, _shapeBuilder);
	}
}

void GridRenderer::clear() {
	 _shapeBuilder.clear();
}

void GridRenderer::render(const video::Camera& camera, const voxel::Region& region) {
	core_trace_scoped(GridRendererRender);

	if (_renderGrid) {
		const glm::vec3& center = glm::vec3(region.getCentre());
		const glm::vec3& halfWidth = glm::vec3(region.getDimensionsInCells()) / 2.0f;
		const core::Plane planeLeft  (glm::left,     center + glm::vec3(-halfWidth.x, 0.0f, 0.0f));
		const core::Plane planeRight (glm::right,    center + glm::vec3( halfWidth.x, 0.0f, 0.0f));
		const core::Plane planeBottom(glm::down,     center + glm::vec3(0.0f, -halfWidth.y, 0.0f));
		const core::Plane planeTop   (glm::up,       center + glm::vec3(0.0f,  halfWidth.y, 0.0f));
		const core::Plane planeNear  (glm::forward,  center + glm::vec3(0.0f, 0.0f, -halfWidth.z));
		const core::Plane planeFar   (glm::backward, center + glm::vec3(0.0f, 0.0f,  halfWidth.z));

		if (planeFar.isBackSide(camera.position())) {
			_shapeRenderer.render(_gridMeshIndexXYFar, camera);
		}
		if (planeNear.isBackSide(camera.position())) {
			_shapeRenderer.render(_gridMeshIndexXYNear, camera);
		}

		if (planeBottom.isBackSide(camera.position())) {
			_shapeRenderer.render(_gridMeshIndexXZNear, camera);
		}
		if (planeTop.isBackSide(camera.position())) {
			_shapeRenderer.render(_gridMeshIndexXZFar, camera);
		}

		if (planeLeft.isBackSide(camera.position())) {
			_shapeRenderer.render(_gridMeshIndexYZNear, camera);
		}
		if (planeRight.isBackSide(camera.position())) {
			_shapeRenderer.render(_gridMeshIndexYZFar, camera);
		}
	} else if (_renderAABB) {
		_shapeRenderer.render(_aabbMeshIndex, camera);
	}
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
