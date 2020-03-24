/**
 * @file
 */

#include "CachedMeshRenderer.h"

namespace voxelrender {

CachedMeshRenderer::CachedMeshRenderer(const voxelformat::MeshCachePtr& meshCache) :
		_meshCache(meshCache) {
}

bool CachedMeshRenderer::init() {
	if (!_meshCache->init()) {
		return false;
	}
	return _meshRenderer.init();
}

void CachedMeshRenderer::shutdown() {
	_meshCache->shutdown();
	_meshRenderer.shutdown();
}

bool CachedMeshRenderer::removeMesh(int index) {
	return _meshRenderer.setMesh(index, nullptr);
}

int CachedMeshRenderer::addMesh(const char *fullpath, const glm::mat4& model) {
	const voxel::Mesh* mesh = _meshCache->getMesh(fullpath);
	if (mesh == nullptr) {
		return -1;
	}
	return _meshRenderer.addMesh(mesh, model);
}

bool CachedMeshRenderer::setModelMatrix(int idx, const glm::mat4& model) {
	return _meshRenderer.setModelMatrix(idx, model);
}

void CachedMeshRenderer::renderAll(const video::Camera& camera) {
	_meshRenderer.renderAll(camera);
}

void CachedMeshRenderer::render(int idx, const video::Camera& camera) {
	_meshRenderer.render(idx, camera);
}

}
