/**
 * @file
 */

#include "ModifierRenderer.h"
#include "core/Color.h"
#include "math/Axis.h"
#include "video/Camera.h"
#include "video/ScopedPolygonMode.h"
#include "voxedit-util/SceneManager.h"
#include "../AxisUtil.h"
#include "voxedit-util/modifier/Selection.h"

namespace voxedit {

bool ModifierRenderer::init() {
	if (!_shapeRenderer.init()) {
		Log::error("Failed to initialize the shape renderer");
		return false;
	}

	return true;
}

void ModifierRenderer::shutdown() {
	_mirrorMeshIndex = -1;
	_aabbMeshIndex = -1;
	_selectionIndex = -1;
	_voxelCursorMesh = -1;
	_shapeRenderer.shutdown();
	_shapeBuilder.shutdown();
}

void ModifierRenderer::updateCursor(const voxel::Voxel& voxel) {
	_shapeBuilder.clear();
	_shapeBuilder.setColor(core::Color::alpha(core::Color::darker(voxel::getMaterialColor(voxel)), 0.6f));
	_shapeBuilder.cube(glm::vec3(-0.01f), glm::vec3(1.01f));
	_shapeRenderer.createOrUpdate(_voxelCursorMesh, _shapeBuilder);
}

void ModifierRenderer::updateSelectionBuffers(const Selection& selection) {
	_shapeBuilder.clear();
	_shapeBuilder.setColor(core::Color::Yellow);
	_shapeBuilder.aabb(selection.getLowerCorner(), selection.getUpperCorner() + glm::one<glm::ivec3>());
	_shapeRenderer.createOrUpdate(_selectionIndex, _shapeBuilder);
}

void ModifierRenderer::updateAABBMirrorMesh(const glm::vec3& mins, const glm::vec3& maxs,
		const glm::vec3& minsMirror, const glm::vec3& maxsMirror) {
	_shapeBuilder.clear();
	_shapeBuilder.setColor(core::Color::alpha(core::Color::Red, 0.5f));
	_shapeBuilder.cube(mins, maxs);
	_shapeBuilder.cube(minsMirror, maxsMirror);
	_shapeRenderer.createOrUpdate(_aabbMeshIndex, _shapeBuilder);
}

void ModifierRenderer::updateAABBMesh(const glm::vec3& mins, const glm::vec3& maxs) {
	_shapeBuilder.clear();
	_shapeBuilder.setColor(core::Color::alpha(core::Color::Red, 0.5f));
	_shapeBuilder.cube(mins, maxs);
	_shapeRenderer.createOrUpdate(_aabbMeshIndex, _shapeBuilder);
}

void ModifierRenderer::renderAABBMode(const video::Camera& camera) {
	static const glm::vec2 offset(-0.25f, -0.5f);
	video::ScopedPolygonMode polygonMode(camera.polygonMode(), offset);
	_shapeRenderer.render(_aabbMeshIndex, camera);
}

void ModifierRenderer::render(const video::Camera& camera, const glm::mat4& model) {
	_shapeRenderer.render(_voxelCursorMesh, camera, model);
	_shapeRenderer.render(_mirrorMeshIndex, camera);
}

void ModifierRenderer::renderSelection(const video::Camera& camera) {
	_shapeRenderer.render(_selectionIndex, camera);
}

void ModifierRenderer::updateMirrorPlane(math::Axis axis, const glm::ivec3& mirrorPos) {
	if (axis == math::Axis::None) {
		if (_mirrorMeshIndex != -1) {
			_shapeRenderer.deleteMesh(_mirrorMeshIndex);
			_mirrorMeshIndex = -1;
		}
		return;
	}

	updateShapeBuilderForPlane(_shapeBuilder, sceneMgr().region(), true, mirrorPos, axis,
			core::Color::alpha(core::Color::LightGray, 0.3f));
	_shapeRenderer.createOrUpdate(_mirrorMeshIndex, _shapeBuilder);
}

}
