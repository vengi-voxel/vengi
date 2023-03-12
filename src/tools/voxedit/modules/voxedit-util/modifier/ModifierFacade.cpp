/**
 * @file
 */

#include "ModifierFacade.h"
#include "voxel/Face.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

namespace voxedit {

bool ModifierFacade::init() {
	return Super::init() && _modifierRenderer.init();
}

void ModifierFacade::shutdown() {
	Super::shutdown();
	_modifierRenderer.shutdown();
}

bool ModifierFacade::setMirrorAxis(math::Axis axis, const glm::ivec3& mirrorPos) {
	if (Super::setMirrorAxis(axis, mirrorPos)) {
		_modifierRenderer.updateMirrorPlane(axis, mirrorPos);
		return true;
	}
	return false;
}

void ModifierFacade::setReferencePosition(const glm::ivec3& pos) {
	Super::setReferencePosition(pos);
	_modifierRenderer.updateReferencePosition(_referencePos);
}

void ModifierFacade::render(const video::Camera& camera) {
	if (_locked) {
		return;
	}
	if (_aabbMode) {
		static glm::ivec3 lastCursor = aabbPosition();
		static math::Axis lastMirrorAxis = _mirrorAxis;

		const glm::ivec3& cursor = aabbPosition();
		const bool needsUpdate = lastCursor != cursor || lastMirrorAxis != _mirrorAxis;

		if (needsUpdate) {
			lastMirrorAxis = _mirrorAxis;
			lastCursor = cursor;
			const math::AABB<int>& bbox = aabb();
			const glm::ivec3& mins = bbox.mins();
			const glm::ivec3& maxs = bbox.maxs();
			glm::ivec3 minsMirror = mins;
			glm::ivec3 maxsMirror = maxs;
			const int size = _gridResolution;
			if (getMirrorAABB(minsMirror, maxsMirror)) {
				const math::AABB<int> first(mins, maxs);
				const math::AABB<int> second(minsMirror, maxsMirror);
				if (math::intersects(first, second)) {
					_modifierRenderer.updateAABBMesh(mins, maxsMirror + size);
				} else {
					_modifierRenderer.updateAABBMirrorMesh(mins, maxs + size, minsMirror, maxsMirror + size);
				}
			} else {
				_modifierRenderer.updateAABBMesh(mins, maxs + size);
			}
		}

		_modifierRenderer.renderAABBMode(camera);
	}
	const glm::ivec3 pos = aabbPosition();
	const glm::mat4& translate = glm::translate(glm::vec3(pos));
	const glm::mat4& scale = glm::scale(translate, glm::vec3((float)_gridResolution));
	const bool flip = voxel::isAir(_voxelAtCursor.getMaterial());
	_modifierRenderer.updateCursor(_cursorVoxel, _face, flip);
	_modifierRenderer.render(camera, scale);
	if (_selectionValid) {
		_modifierRenderer.renderSelection(camera);
	}
}

bool ModifierFacade::select(const glm::ivec3& mins, const glm::ivec3& maxs) {
	if (Super::select(mins, maxs)) {
		_modifierRenderer.updateSelectionBuffers(_selection);
		return true;
	}
	return false;
}

void ModifierFacade::invert(const voxel::Region &region) {
	if (_locked) {
		return;
	}
	Super::invert(region);
	if (_selectionValid) {
		_modifierRenderer.updateSelectionBuffers(_selection);
	}
}

void ModifierFacade::unselect() {
	if (_locked) {
		return;
	}
	Super::unselect();
	if (_selectionValid) {
		_modifierRenderer.updateSelectionBuffers(_selection);
	}
}

}
