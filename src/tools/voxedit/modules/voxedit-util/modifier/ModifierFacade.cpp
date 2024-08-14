/**
 * @file
 */

#include "ModifierFacade.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/ModifierType.h"
#include "voxedit-util/modifier/brush/AABBBrush.h"
#include "voxedit-util/modifier/brush/BrushType.h"
#include "voxel/RawVolume.h"
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/transform.hpp>

namespace voxedit {

ModifierFacade::ModifierFacade(SceneManager *sceneMgr, const ModifierRendererPtr &modifierRenderer)
	: Super(sceneMgr), _modifierRenderer(modifierRenderer), _sceneMgr(sceneMgr) {
}

bool ModifierFacade::init() {
	if (!Super::init()) {
		return false;
	}
	return _modifierRenderer->init();
}

void ModifierFacade::shutdown() {
	Super::shutdown();
	// the volumes of the renderer are not deleted by this shutdown
	// call, but with our scoped pointers
	_modifierRenderer->shutdown();
}

static void createOrClearPreviewVolume(voxel::RawVolume *existingVolume, core::ScopedPtr<voxel::RawVolume> &volume, voxel::Region region) {
	if (existingVolume == nullptr) {
		if (volume == nullptr || volume->region() != region) {
			volume = new voxel::RawVolume(region);
			return;
		}
		volume->clear();
	} else {
		region.grow(1);
		volume = new voxel::RawVolume(*existingVolume, region);
	}
}

bool ModifierFacade::previewNeedsExistingVolume() const {
	if (isMode(ModifierType::Paint)) {
		return true;
	}
	if (_brushType == BrushType::Plane) {
		return isMode(ModifierType::Place);
	}
	return false;
}

void ModifierFacade::updateBrushVolumePreview(palette::Palette &activePalette) {
	// even in erase mode we want the preview to create the models, not wipe them
	ModifierType modifierType = _brushContext.modifierType;
	if (modifierType == ModifierType::Erase) {
		modifierType = ModifierType::Place;
	}
	voxel::Voxel voxel = _brushContext.cursorVoxel;
	voxel.setOutline();

	// this call is needed to prevent double frees
	_modifierRenderer->clearBrushMeshes();

	Log::debug("regenerate preview volume");

	scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();
	voxel::RawVolume *activeVolume = _sceneMgr->volume(sceneGraph.activeNode());
	if (activeVolume == nullptr) {
		return;
	}

	// operate on existing voxels
	voxel::RawVolume *existingVolume = nullptr;
	if (previewNeedsExistingVolume()) {
		existingVolume = activeVolume;
	}

	// TODO: for erase we have to use the existing volume
	// and hide the real volume to show the modification only.
	if (const Brush *brush = currentBrush()) {
		preExecuteBrush(activeVolume);
		const voxel::Region &region = brush->calcRegion(_brushContext);
		if (region.isValid()) {
			glm::ivec3 minsMirror = region.getLowerCorner();
			glm::ivec3 maxsMirror = region.getUpperCorner();
			if (brush->getMirrorAABB(minsMirror, maxsMirror)) {
				createOrClearPreviewVolume(existingVolume, _mirrorVolume, voxel::Region(minsMirror, maxsMirror));
				scenegraph::SceneGraphNode mirrorDummyNode(scenegraph::SceneGraphNodeType::Model);
				mirrorDummyNode.setVolume(_mirrorVolume, false);
				executeBrush(sceneGraph, mirrorDummyNode, modifierType, voxel);
				_modifierRenderer->updateBrushVolume(1, _mirrorVolume, &activePalette);
			}
			createOrClearPreviewVolume(existingVolume, _volume, region);
			scenegraph::SceneGraphNode dummyNode(scenegraph::SceneGraphNodeType::Model);
			dummyNode.setVolume(_volume, false);
			executeBrush(sceneGraph, dummyNode, modifierType, voxel);
			_modifierRenderer->updateBrushVolume(0, _volume, &activePalette);
		}
		postExecuteBrush();
	}
}

void ModifierFacade::render(const video::Camera &camera, palette::Palette &activePalette) {
	if (_locked) {
		return;
	}
	const glm::mat4 &translate = glm::translate(glm::vec3(_brushContext.cursorPosition));
	const glm::mat4 &scale = glm::scale(translate, glm::vec3((float)_brushContext.gridResolution));
	const bool flip = voxel::isAir(_brushContext.voxelAtCursor.getMaterial());
	_modifierRenderer->updateCursor(_brushContext.cursorVoxel, _brushContext.cursorFace, flip);
	Brush *brush = currentBrush();
	if (brush) {
		int activeNode = _sceneMgr->sceneGraph().activeNode();
		if (activeNode != InvalidNodeId) {
			if (const scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphModelNode(activeNode)) {
				_modifierRenderer->updateMirrorPlane(brush->mirrorAxis(), brush->mirrorPos(), node->region());
			}
		}
	}
	_modifierRenderer->updateReferencePosition(referencePosition());
	_modifierRenderer->render(camera, scale);

	// TODO: SELECTION: remove me - let the SelectionManager render the SparseVolume
	if (_brushType == BrushType::Select && brush->active()) {
		if (brush->dirty()) {
			const voxel::Region &region = brush->calcRegion(_brushContext);
			Selections selections = selectionMgr().selections();
			selections.push_back(region);
			_modifierRenderer->updateSelectionBuffers(selections);
			brush->markClean();
		}
	} else {
		_modifierRenderer->updateSelectionBuffers(selectionMgr().selections());
	}
	_modifierRenderer->renderSelection(camera);

	if (isMode(ModifierType::ColorPicker)) {
		return;
	}

	if (brush && brush->active()) {
		if (brush->dirty()) {
			updateBrushVolumePreview(activePalette);
			brush->markClean();
		}
		video::polygonOffset(glm::vec3(-0.1f));
		_modifierRenderer->renderBrushVolume(camera);
		video::polygonOffset(glm::vec3(0.0f));
	} else {
		_modifierRenderer->clearBrushMeshes();
	}
}

} // namespace voxedit
