/**
 * @file
 */

#include "ModifierFacade.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/ModifierType.h"
#include "voxedit-util/modifier/SelectionManager.h"
#include "voxedit-util/modifier/brush/BrushType.h"
#include "voxedit-util/modifier/brush/ShapeBrush.h"
#include "voxel/RawVolume.h"
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/transform.hpp>

namespace voxedit {

ModifierFacade::ModifierFacade(SceneManager *sceneMgr, const ModifierRendererPtr &modifierRenderer, const SelectionManagerPtr &selectionManager)
	: Super(sceneMgr, selectionManager), _modifierRenderer(modifierRenderer), _sceneMgr(sceneMgr) {
}

bool ModifierFacade::init() {
	if (!Super::init()) {
		return false;
	}
	_maxSuggestedVolumeSizePreview = core::Var::getSafe(cfg::VoxEditMaxSuggestedVolumeSizePreview);
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

bool ModifierFacade::generateSimplePreview(const Brush *brush, const voxel::Region &region) const {
	if (brush->type() != BrushType::Shape) {
		return false;
	}
	const ShapeBrush *shapeBrush = static_cast<const ShapeBrush *>(brush);
	if (shapeBrush->shapeType() == ShapeType::AABB) {
		// we can use a simple cube for the preview here
		return true;
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
	_modifierRenderer->clear();

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

	// TODO: BRUSH: for erase we have to use the existing volume
	// and hide the real volume to show the modification only.
	const Brush *brush = currentBrush();
	if (!brush) {
		return;
	}
	preExecuteBrush(activeVolume);
	const voxel::Region &region = brush->calcRegion(_brushContext);
	if (!region.isValid()) {
		return;
	}
	const voxel::Region maxPreviewRegion(0, _maxSuggestedVolumeSizePreview->intVal() - 1);
	bool simplePreview = generateSimplePreview(brush, region);
	if (!simplePreview && region.voxels() < maxPreviewRegion.voxels()) {
		glm::ivec3 minsMirror = region.getLowerCorner();
		glm::ivec3 maxsMirror = region.getUpperCorner();
		if (brush->getMirrorAABB(minsMirror, maxsMirror)) {
			createOrClearPreviewVolume(existingVolume, _previewMirrorVolume, voxel::Region(minsMirror, maxsMirror));
			scenegraph::SceneGraphNode mirrorDummyNode(scenegraph::SceneGraphNodeType::Model);
			mirrorDummyNode.setVolume(_previewMirrorVolume, false);
			executeBrush(sceneGraph, mirrorDummyNode, modifierType, voxel);
			_modifierRenderer->updateBrushVolume(1, _previewMirrorVolume, &activePalette);
		}
		createOrClearPreviewVolume(existingVolume, _previewVolume, region);
		scenegraph::SceneGraphNode dummyNode(scenegraph::SceneGraphNodeType::Model);
		dummyNode.setVolume(_previewVolume, false);
		executeBrush(sceneGraph, dummyNode, modifierType, voxel);
		_modifierRenderer->updateBrushVolume(0, _previewVolume, &activePalette);
	} else if (simplePreview) {
		_modifierRenderer->updateBrushVolume(0, nullptr, nullptr);
		_modifierRenderer->updateBrushVolume(1, nullptr, nullptr);
		glm::ivec3 minsMirror = region.getLowerCorner();
		glm::ivec3 maxsMirror = region.getUpperCorner();
		color::RGBA color = activePalette.color(_brushContext.cursorVoxel.getColor());
		if (brush->getMirrorAABB(minsMirror, maxsMirror)) {
			_modifierRenderer->updateBrushVolume(1, {minsMirror, maxsMirror}, color);
		}
		_modifierRenderer->updateBrushVolume(0, region, color);
	}
}

void ModifierFacade::render(const video::Camera &camera, palette::Palette &activePalette, const glm::mat4 &model) {
	if (_locked) {
		return;
	}
	const glm::mat4 &translate = glm::translate(model, glm::vec3(_brushContext.cursorPosition));
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
	_modifierRenderer->render(camera, scale, model);

	// TODO: SELECTION: remove me - let the SelectionManager render this
	int activeNodeId = _sceneMgr->sceneGraph().activeNode();
	const scenegraph::SceneGraphNode *activeModelNode = _sceneMgr->sceneGraphModelNode(activeNodeId);
	if (_brushType == BrushType::Select && brush->active()) {
		if (brush->dirty()) {
			const voxel::Region &region = brush->calcRegion(_brushContext);
			scenegraph::Selections selections;
			if (activeModelNode) {
				selections = activeModelNode->selections();
			}
			selections.push_back(region);
			_modifierRenderer->updateSelectionBuffers(selections);
			brush->markClean();
		}
	} else {
		if (activeModelNode) {
			_modifierRenderer->updateSelectionBuffers(activeModelNode->selections());
		} else {
			_modifierRenderer->updateSelectionBuffers({});
		}
	}
	_modifierRenderer->renderSelection(camera, model);

	if (isMode(ModifierType::ColorPicker)) {
		return;
	}

	if (brush && brush->active()) {
		if (brush->dirty()) {
			if (_nextPreviewUpdateSeconds > 0.0) {
				_nextPreviewUpdateSeconds -= 0.02f;
			} else {
				_nextPreviewUpdateSeconds = _nowSeconds + 0.1f;
			}
			brush->markClean();
		}
		if (_nextPreviewUpdateSeconds > 0.0) {
			if (_nextPreviewUpdateSeconds <= _nowSeconds) {
				_nextPreviewUpdateSeconds = 0.0f;
				updateBrushVolumePreview(activePalette);
			}
		}
		video::polygonOffset(glm::vec3(-0.1f));
		_modifierRenderer->renderBrushVolume(camera, model);
		video::polygonOffset(glm::vec3(0.0f));
	} else {
		_modifierRenderer->clear();
	}
}

} // namespace voxedit
