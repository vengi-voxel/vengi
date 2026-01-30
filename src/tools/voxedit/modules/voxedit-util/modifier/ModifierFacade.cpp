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
#include "voxedit-util/modifier/brush/BrushType.h"
#include "voxedit-util/modifier/brush/ShapeBrush.h"
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
	_maxSuggestedVolumeSizePreview = core::Var::getSafe(cfg::VoxEditMaxSuggestedVolumeSizePreview);
	return _modifierRenderer->init();
}

void ModifierFacade::shutdown() {
	Super::shutdown();
	_modifierRenderer->shutdown();
}

static void createOrClearPreviewVolume(voxel::RawVolume *existingVolume, core::ScopedPtr<voxel::RawVolume> &volume,
									   voxel::Region region) {
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
	if (isMode(ModifierType::Select)) {
		return true;
	}
	if (_brushType == BrushType::Plane) {
		return isMode(ModifierType::Place);
	}
	return false;
}

bool ModifierFacade::isSimplePreview(const Brush *brush, const voxel::Region &region) const {
	if (brush->type() == BrushType::Select) {
		// Selection brush just needs to show the region that will be selected
		return true;
	}
	if (brush->type() != BrushType::Shape) {
		return false;
	}
	const ShapeBrush *shapeBrush = (const ShapeBrush *)brush;
	if (shapeBrush->shapeType() == ShapeType::AABB) {
		// we can use a simple cube for the preview here
		return true;
	}
	return false;
}

void ModifierFacade::updateBrushVolumePreview(palette::Palette &activePalette, ModifierRendererContext &ctx) {
	// even in erase mode we want the preview to create the models, not wipe them
	ModifierType modifierType = _brushContext.modifierType;
	if (modifierType == ModifierType::Erase) {
		modifierType = ModifierType::Place;
	}
	voxel::Voxel voxel = _brushContext.cursorVoxel;
	voxel.setOutline();

	// Reset preview state
	_previewVolume = nullptr;
	_previewMirrorVolume = nullptr;
	ctx.previewVolume = nullptr;
	ctx.previewMirrorVolume = nullptr;
	ctx.simplePreviewRegion = voxel::Region::InvalidRegion;
	ctx.simpleMirrorPreviewRegion = voxel::Region::InvalidRegion;
	ctx.useSimplePreview = false;

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
	bool simplePreview = isSimplePreview(brush, region);
	if (!simplePreview && region.voxels() < maxPreviewRegion.voxels()) {
		glm::ivec3 minsMirror = region.getLowerCorner();
		glm::ivec3 maxsMirror = region.getUpperCorner();
		if (brush->getMirrorAABB(minsMirror, maxsMirror)) {
			createOrClearPreviewVolume(existingVolume, _previewMirrorVolume, voxel::Region(minsMirror, maxsMirror));
			scenegraph::SceneGraphNode mirrorDummyNode(scenegraph::SceneGraphNodeType::Model);
			mirrorDummyNode.setVolume(_previewMirrorVolume, false);
			executeBrush(sceneGraph, mirrorDummyNode, modifierType, voxel);
			ctx.previewMirrorVolume = _previewMirrorVolume;
		}
		createOrClearPreviewVolume(existingVolume, _previewVolume, region);
		scenegraph::SceneGraphNode dummyNode(scenegraph::SceneGraphNodeType::Model);
		dummyNode.setVolume(_previewVolume, false);
		executeBrush(sceneGraph, dummyNode, modifierType, voxel);
		ctx.previewVolume = _previewVolume;
		ctx.palette = &activePalette;
	} else if (simplePreview) {
		ctx.useSimplePreview = true;
		ctx.simplePreviewRegion = region;
		ctx.simplePreviewColor = activePalette.color(_brushContext.cursorVoxel.getColor());
		glm::ivec3 minsMirror = region.getLowerCorner();
		glm::ivec3 maxsMirror = region.getUpperCorner();
		if (brush->getMirrorAABB(minsMirror, maxsMirror)) {
			ctx.simpleMirrorPreviewRegion = voxel::Region(minsMirror, maxsMirror);
		}
	}
}

void ModifierFacade::render(const video::Camera &camera, palette::Palette &activePalette, const glm::mat4 &model) {
	if (_locked) {
		return;
	}

	scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();
	const int activeNodeId = sceneGraph.activeNode();
	Brush *brush = currentBrush();

	// Build the context for the renderer
	ModifierRendererContext ctx;
	ctx.cursorVoxel = _brushContext.cursorVoxel;
	ctx.voxelAtCursor = _brushContext.voxelAtCursor;
	ctx.cursorFace = _brushContext.cursorFace;
	ctx.cursorPosition = _brushContext.cursorPosition;
	ctx.gridResolution = _brushContext.gridResolution;
	ctx.referencePosition = referencePosition();
	ctx.palette = &activePalette;
	ctx.brushActive = false;

	// Mirror plane info
	if (brush) {
		ctx.mirrorAxis = brush->mirrorAxis();
		ctx.mirrorPos = brush->mirrorPos();
	}
	if (const scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphModelNode(activeNodeId)) {
		ctx.activeRegion = node->region();
	}

	// Handle brush preview with deferred updates
	if (!isMode(ModifierType::ColorPicker)) {
		ctx.brushActive = brush && brush->active();
	}

	if (ctx.brushActive) {
		if (brush->dirty()) {
			if (_nextPreviewUpdateSeconds > 0.0) {
				_nextPreviewUpdateSeconds -= 0.02;
			} else {
				_nextPreviewUpdateSeconds = _nowSeconds + 0.1;
			}
			brush->markClean();
		}
		if (_nextPreviewUpdateSeconds > 0.0) {
			if (_nextPreviewUpdateSeconds <= _nowSeconds) {
				_nextPreviewUpdateSeconds = 0.0;
				updateBrushVolumePreview(activePalette, ctx);
			}
		}
		// If we have preview data from previous update, pass it along
		if (!ctx.previewVolume && !ctx.useSimplePreview) {
			ctx.previewVolume = _previewVolume;
			ctx.previewMirrorVolume = _previewMirrorVolume;
			ctx.palette = &activePalette;
		}
	}

	// Let the renderer handle buffer updates and rendering
	_modifierRenderer->update(ctx);

	// Render everything
	_modifierRenderer->render(camera, model);
}

} // namespace voxedit
