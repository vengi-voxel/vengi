/**
 * @file
 */

#include "ModifierFacade.h"
#include "core/ScopedPtr.h"
#include "voxedit-util/modifier/ModifierType.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

namespace voxedit {

ModifierFacade::ModifierFacade(const ModifierRendererPtr &modifierRenderer) : _modifierRenderer(modifierRenderer) {
}

bool ModifierFacade::init() {
	if (!Super::init()) {
		return false;
	}
	return _modifierRenderer->init();
}

void ModifierFacade::shutdown() {
	Super::shutdown();
	_modifierRenderer->shutdown();
}

bool ModifierFacade::setMirrorAxis(math::Axis axis, const glm::ivec3 &mirrorPos) {
	if (Super::setMirrorAxis(axis, mirrorPos)) {
		_modifierRenderer->updateMirrorPlane(axis, mirrorPos);
		return true;
	}
	return false;
}

void ModifierFacade::setReferencePosition(const glm::ivec3 &pos) {
	Super::setReferencePosition(pos);
	_modifierRenderer->updateReferencePosition(_referencePos);
}

void ModifierFacade::updateShapeRenderPreview(voxel::Palette &palette) {
	const math::AABB<int> &bbox = aabb();
	const glm::ivec3 &mins = bbox.mins();
	const glm::ivec3 &maxs = bbox.maxs();
	glm::ivec3 minsMirror = mins;
	glm::ivec3 maxsMirror = maxs;
	_modifierRenderer->clearShapeMeshes();

	// even in erase mode we want the preview to create the models, not wipe them
	ModifierType modifierType = _modifierType;
	if ((modifierType & ModifierType::Erase) == ModifierType::Erase) {
		modifierType &= ~ModifierType::Erase;
	}

	if (getMirrorAABB(minsMirror, maxsMirror)) {
		_mirrorVolume = new voxel::RawVolume(voxel::Region(minsMirror, maxsMirror));
		runModifier(_mirrorVolume, modifierType);
		_modifierRenderer->updateShapeMesh(1, _mirrorVolume, &palette);
	}
	_volume = new voxel::RawVolume(voxel::Region(mins, maxs));
	runModifier(_volume, modifierType);
	_modifierRenderer->updateShapeMesh(0, _volume, &palette);
}

void ModifierFacade::render(const video::Camera &camera, voxel::Palette &palette) {
	if (_locked) {
		return;
	}
	const glm::ivec3 &pos = aabbPosition();
	const glm::mat4 &translate = glm::translate(glm::vec3(pos));
	const glm::mat4 &scale = glm::scale(translate, glm::vec3((float)_gridResolution));
	const bool flip = voxel::isAir(_voxelAtCursor.getMaterial());
	_modifierRenderer->updateCursor(_cursorVoxel, _face, flip);
	_modifierRenderer->render(camera, scale);
	if (_selectionValid) {
		_modifierRenderer->renderSelection(camera);
	}
	if (_aabbMode) {
		static glm::ivec3 lastCursor = aabbPosition();
		static math::Axis lastMirrorAxis = _mirrorAxis;

		const bool needsRenderUpdate = lastCursor != pos || lastMirrorAxis != _mirrorAxis;
		if (needsRenderUpdate) {
			lastMirrorAxis = _mirrorAxis;
			lastCursor = pos;
			updateShapeRenderPreview(palette);
		}
		_modifierRenderer->renderShape(camera);
	}
}

bool ModifierFacade::select(const glm::ivec3 &mins, const glm::ivec3 &maxs) {
	if (Super::select(mins, maxs)) {
		_modifierRenderer->updateSelectionBuffers(_selections);
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
		_modifierRenderer->updateSelectionBuffers(_selections);
	}
}

void ModifierFacade::unselect() {
	if (_locked) {
		return;
	}
	Super::unselect();
	if (_selectionValid) {
		_modifierRenderer->updateSelectionBuffers(_selections);
	}
}

} // namespace voxedit
