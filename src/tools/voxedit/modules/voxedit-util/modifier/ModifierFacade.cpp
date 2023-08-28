/**
 * @file
 */

#include "ModifierFacade.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/ModifierType.h"
#include "voxedit-util/modifier/ShapeType.h"
#include "voxedit-util/modifier/brush/ShapeBrush.h"
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

void ModifierFacade::updateBrushVolumePreview(voxel::Palette &palette) {
	// even in erase mode we want the preview to create the models, not wipe them
	ModifierType modifierType = _modifierType;
	if ((modifierType & ModifierType::Erase) == ModifierType::Erase) {
		modifierType &= ~ModifierType::Erase;
	}
	voxel::Voxel voxel = _brushContext.cursorVoxel;
	voxel.setOutline();

	// this call is needed to prevent double frees
	_modifierRenderer->clearBrushMeshes();

	switch (_brushType) {
	case BrushType::Shape: {
		const ShapeBrush *brush = activeShapeBrush();
		const voxel::Region &region = brush->calcRegion(_brushContext);
		glm::ivec3 minsMirror = region.getLowerCorner();
		glm::ivec3 maxsMirror = region.getUpperCorner();
		if (brush->getMirrorAABB(minsMirror, maxsMirror)) {
			_mirrorVolume = new voxel::RawVolume(voxel::Region(minsMirror, maxsMirror));
			runModifier(sceneMgr().sceneGraph(), _mirrorVolume, modifierType, voxel);
			_modifierRenderer->updateBrushVolume(1, _mirrorVolume, &palette);
		}
		_volume = new voxel::RawVolume(region);
		runModifier(sceneMgr().sceneGraph(), _volume, modifierType, voxel);
		_modifierRenderer->updateBrushVolume(0, _volume, &palette);
		break;
	}
	case BrushType::Stamp: {
		voxel::RawVolume *v = _stampBrush.volume();
		if (v != nullptr) {
			const voxel::Region &region = _stampBrush.calcRegion(_brushContext);
			_volume = new voxel::RawVolume(region);
			runModifier(sceneMgr().sceneGraph(), _volume, modifierType, voxel);
			// TODO: use _stampBrush palette?
			_modifierRenderer->updateBrushVolume(0, _volume, &palette);
		}
		break;
	}
	default:
		break;
	}
}

void ModifierFacade::render(const video::Camera &camera, voxel::Palette &palette) {
	if (_locked) {
		return;
	}
	const glm::mat4 &translate = glm::translate(glm::vec3(_brushContext.cursorPosition));
	const glm::mat4 &scale = glm::scale(translate, glm::vec3((float)_brushContext.gridResolution));
	const bool flip = voxel::isAir(_brushContext.voxelAtCursor.getMaterial());
	_modifierRenderer->updateCursor(_brushContext.cursorVoxel, _brushContext.cursorFace, flip);
	_modifierRenderer->updateMirrorPlane(shapeBrush().mirrorAxis(), shapeBrush().mirrorPos());
	_modifierRenderer->updateReferencePosition(referencePosition());
	_modifierRenderer->render(camera, scale);
	if (_selectionValid) {
		_modifierRenderer->renderSelection(camera);
	}

	if (isMode(ModifierType::ColorPicker) || isMode(ModifierType::Select) || isMode(ModifierType::Line) ||
		isMode(ModifierType::Path)) {
		return;
	}

	Brush *brush = activeBrush();
	if (brush && brush->active()) {
		if (brush->dirty()) {
			brush->markClean();
			updateBrushVolumePreview(palette);
		}
		_modifierRenderer->renderBrushVolume(camera);
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
