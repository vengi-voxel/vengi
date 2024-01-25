/**
 * @file
 */

#include "ModifierFacade.h"
#include "core/ScopedPtr.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/ModifierType.h"
#include "voxedit-util/modifier/brush/AABBBrush.h"
#include "voxel/RawVolume.h"
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
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

static voxel::RawVolume *createPreviewVolume(const voxel::RawVolume *existingVolume, const voxel::Region &region) {
	if (existingVolume == nullptr) {
		return new voxel::RawVolume(region);
	}
	return new voxel::RawVolume(*existingVolume, region);
}

void ModifierFacade::updateBrushVolumePreview(palette::Palette &palette) {
	// even in erase mode we want the preview to create the models, not wipe them
	ModifierType modifierType = _modifierType;
	if (modifierType == ModifierType::Erase) {
		modifierType = ModifierType::Place;
	}
	voxel::Voxel voxel = _brushContext.cursorVoxel;
	voxel.setOutline();

	// this call is needed to prevent double frees
	_modifierRenderer->clearBrushMeshes();

	scenegraph::SceneGraph &sceneGraph = sceneMgr().sceneGraph();
	voxel::RawVolume *existingVolume = nullptr;
	if (_modifierType == ModifierType::Paint) {
		existingVolume = sceneMgr().volume(sceneGraph.activeNode());
	}
	const AABBBrush *aabbBrush = activeAABBBrush();
	if (aabbBrush) {
		const voxel::Region &region = aabbBrush->calcRegion(_brushContext);
		glm::ivec3 minsMirror = region.getLowerCorner();
		glm::ivec3 maxsMirror = region.getUpperCorner();
		if (aabbBrush->getMirrorAABB(minsMirror, maxsMirror)) {
			_mirrorVolume = createPreviewVolume(existingVolume, voxel::Region(minsMirror, maxsMirror));
			scenegraph::SceneGraphNode mirrorDummyNode(scenegraph::SceneGraphNodeType::Model);
			mirrorDummyNode.setVolume(_mirrorVolume, false);
			executeBrush(sceneGraph, mirrorDummyNode, modifierType, voxel);
			_modifierRenderer->updateBrushVolume(1, _mirrorVolume, &palette);
		}
		_volume = createPreviewVolume(existingVolume, region);
		scenegraph::SceneGraphNode dummyNode(scenegraph::SceneGraphNodeType::Model);
		dummyNode.setVolume(_volume, false);
		executeBrush(sceneGraph, dummyNode, modifierType, voxel);
		_modifierRenderer->updateBrushVolume(0, _volume, &palette);
	} else {
		switch (_brushType) {
		case BrushType::Stamp: {
			voxel::RawVolume *v = _stampBrush.volume();
			if (v != nullptr) {
				const voxel::Region &region = _stampBrush.calcRegion(_brushContext);
				_volume = new voxel::RawVolume(region);
				scenegraph::SceneGraphNode dummyNode(scenegraph::SceneGraphNodeType::Model);
				dummyNode.setVolume(_volume, false);
				executeBrush(sceneGraph, dummyNode, modifierType, voxel);
				// TODO: support mirror axis
				// TODO: use _stampBrush palette?
				_modifierRenderer->updateBrushVolume(0, _volume, &palette);
			}
			break;
		}
		case BrushType::Line: {
			const voxel::Region region = _lineBrush.calcRegion(_brushContext);
			if (region.isValid()) {
				_volume = new voxel::RawVolume(region);
				scenegraph::SceneGraphNode dummyNode(scenegraph::SceneGraphNodeType::Model);
				dummyNode.setVolume(_volume, false);
				executeBrush(sceneGraph, dummyNode, modifierType, voxel);
				_modifierRenderer->updateBrushVolume(0, _volume, &palette);
			}
			break;
		}
		// TODO: path preview
		default:
			break;
		}
	}
}

void ModifierFacade::render(const video::Camera &camera, palette::Palette &palette) {
	if (_locked) {
		return;
	}
	const glm::mat4 &translate = glm::translate(glm::vec3(_brushContext.cursorPosition));
	const glm::mat4 &scale = glm::scale(translate, glm::vec3((float)_brushContext.gridResolution));
	const bool flip = voxel::isAir(_brushContext.voxelAtCursor.getMaterial());
	_modifierRenderer->updateCursor(_brushContext.cursorVoxel, _brushContext.cursorFace, flip);
	AABBBrush *aabbBrush = activeAABBBrush();
	if (aabbBrush) {
		_modifierRenderer->updateMirrorPlane(aabbBrush->mirrorAxis(), aabbBrush->mirrorPos());
	} else {
		_modifierRenderer->updateMirrorPlane(math::Axis::None, glm::ivec3(0));
	}
	_modifierRenderer->updateReferencePosition(referencePosition());
	_modifierRenderer->render(camera, scale);

	if (isMode(ModifierType::Select) && _selectStartPositionValid) {
		const voxel::Region &region = calcSelectionRegion();
		Selections selections = _selections;
		if (region.isValid()) {
			selections.push_back(region);
		} else if (!_selectionValid) {
			return;
		}
		_modifierRenderer->updateSelectionBuffers(selections);
		_modifierRenderer->renderSelection(camera);
		return;
	}

	if (_selectionValid) {
		_modifierRenderer->updateSelectionBuffers(_selections);
		_modifierRenderer->renderSelection(camera);
	}

	if (isMode(ModifierType::ColorPicker)) {
		return;
	}

	Brush *brush = activeBrush();
	if (brush && brush->active()) {
		if (brush->dirty()) {
			brush->markClean();
			updateBrushVolumePreview(palette);
		}
		video::polygonOffset(glm::vec3(-0.1f));
		_modifierRenderer->renderBrushVolume(camera);
		video::polygonOffset(glm::vec3(0.0f));
	}
}

} // namespace voxedit
