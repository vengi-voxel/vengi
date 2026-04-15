/**
 * @file
 */

#include "PreviewManager.h"
#include "Modifier.h"
#include "core/Log.h"
#include "core/Trace.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/ModelNodeSettings.h"
#include "voxedit-util/modifier/brush/AABBBrush.h"
#include "voxedit-util/modifier/brush/BrushType.h"

namespace voxedit {

void PreviewManager::construct() {
	const core::VarDef voxEditMaxSuggestedVolumeSizePreview(cfg::VoxEditMaxSuggestedVolumeSizePreview, 32, 16,
															(int)voxedit::MaxVolumeSize,
															N_("Max preview size"),
															N_("The maximum size of the preview volume"), -1);
	_maxSuggestedVolumeSizePreview = core::Var::registerVar(voxEditMaxSuggestedVolumeSizePreview);
}

bool PreviewManager::init(const ModifierRendererPtr &renderer) {
	_modifierRenderer = renderer;
	return true;
}

void PreviewManager::shutdown() {
	resetPreview();
}

void PreviewManager::resetPreview() {
	// Clear renderer volume pointers before freeing the volumes.
	// The allocator may reuse the same heap address for the next preview volume,
	// which would cause MeshState::setVolume() to see old == new and skip the
	// update, leaving stale mesh data in GPU buffers.
	if (_modifierRenderer) {
		_modifierRenderer->clearBrushVolumes();
	}
	_previewVolume = nullptr;
	_previewMirrorVolume = nullptr;
	_brushPreview = BrushPreview();
}

void PreviewManager::scheduleUpdate(double nowSeconds) {
	_nextPreviewUpdateSeconds = nowSeconds;
}

bool PreviewManager::checkPendingUpdate(double nowSeconds, Modifier &modifier, palette::Palette &activePalette,
										voxel::RawVolume *activeVolume, scenegraph::SceneGraph &sceneGraph) {
	if (_nextPreviewUpdateSeconds > 0.0 && _nextPreviewUpdateSeconds <= nowSeconds) {
		_nextPreviewUpdateSeconds = 0.0;
		updateBrushVolumePreview(modifier, activePalette, activeVolume, sceneGraph);
		return true;
	}
	return false;
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
		// Keep the old volume alive during allocation so the heap allocator
		// cannot reuse the same address. MeshState::setVolume() compares raw
		// pointers (old == new) and skips the update when they match, which
		// would leave stale mesh data in GPU buffers.
		voxel::RawVolume *old = volume.release();
		volume = new voxel::RawVolume(*existingVolume, region);
		delete old;
	}
}

static bool canAllocatePreviewRegion(const voxel::Region &region, int maxSuggestedExtent) {
	if (!region.isValid()) {
		return false;
	}
	const glm::ivec3 &dimensions = region.getDimensionsInVoxels();
	if (dimensions.x <= 0 || dimensions.y <= 0 || dimensions.z <= 0) {
		return false;
	}
	const int64_t maxExtent = (int64_t)maxSuggestedExtent;
	const int64_t maxVoxels = maxExtent * maxExtent * maxExtent;
	const int64_t voxels = (int64_t)dimensions.x * (int64_t)dimensions.y * (int64_t)dimensions.z;
	return voxels > 0 && voxels <= maxVoxels;
}

bool PreviewManager::previewNeedsExistingVolume(const Modifier &modifier) const {
	if (modifier.isMode(ModifierType::Paint)) {
		return true;
	}
	const BrushType brushType = modifier.brushType();
	if (brushType == BrushType::Select) {
		return true;
	}
	if (brushType == BrushType::Script) {
		const LUABrush *luaBrush = (const LUABrush *)modifier.currentBrush();
		if (luaBrush && luaBrush->previewNeedsExistingVolume()) {
			return true;
		}
	}
	if (brushType == BrushType::Plane) {
		return modifier.isMode(ModifierType::Place);
	}
	if (brushType == BrushType::Extrude) {
		return true;
	}
	if (brushType == BrushType::Transform) {
		return true;
	}
	if (brushType == BrushType::Sculpt) {
		return true;
	}
	return false;
}

bool PreviewManager::isSimplePreview(const Brush *brush, const voxel::Region &region) const {
	if (brush->type() == BrushType::Script) {
		const LUABrush *luaBrush = (const LUABrush *)brush;
		if (luaBrush->useSimplePreview()) {
			return true;
		}
	}
	if (brush->type() == BrushType::Shape) {
		const ShapeBrush *shapeBrush = (const ShapeBrush *)brush;
		if (shapeBrush->shapeType() == ShapeType::AABB) {
			return true;
		}
	}
	if (brush->type() == BrushType::Select) {
		const SelectBrush *selectBrush = (const SelectBrush *)brush;
		const SelectMode mode = selectBrush->selectMode();
		if (mode == SelectMode::All || mode == SelectMode::Surface || mode == SelectMode::Box3D ||
			mode == SelectMode::SameColor || mode == SelectMode::FuzzyColor) {
			return true;
		}
	}
	if (brush->type() == BrushType::Sculpt) {
		const SculptBrush *sculptBrush = (const SculptBrush *)brush;
		if (sculptBrush->sculptMode() == SculptMode::ExtendPlane && sculptBrush->planeFitted()) {
			return true;
		}
	}
	return false;
}

void PreviewManager::updateBrushVolumePreview(Modifier &modifier, palette::Palette &activePalette,
											  voxel::RawVolume *activeVolume,
											  scenegraph::SceneGraph &sceneGraph) {
	// even in erase mode we want the preview to create the models, not wipe them
	const BrushContext &brushContext = modifier.brushContext();
	ModifierType modifierType = brushContext.modifierType;
	if (modifierType == ModifierType::Erase) {
		modifierType = ModifierType::Place;
	}
	voxel::Voxel voxel = brushContext.cursorVoxel;
	voxel.setOutline();

	// Allow subclasses to wait for pending operations before freeing old preview volumes
	_modifierRenderer->waitForPendingExtractions();

	// Reset preview state
	resetPreview();

	Log::debug("regenerate preview volume");

	if (activeVolume == nullptr) {
		return;
	}

	// operate on existing voxels
	voxel::RawVolume *existingVolume = nullptr;
	if (previewNeedsExistingVolume(modifier)) {
		existingVolume = activeVolume;
	}

	const Brush *brush = modifier.currentBrush();
	if (!brush) {
		return;
	}

	// Safety net: brushes with pending changes are handled in render() by
	// executing on the real volume. If we reach here anyway, bail out to
	// avoid corrupting brush history state against a temporary dummy volume.
	if (brush->hasPendingChanges()) {
		return;
	}

	modifier.preExecuteBrush(activeVolume);
	const voxel::Region &region = brush->calcRegion(brushContext);
	if (!region.isValid()) {
		return;
	}
	const int maxPreviewExtent = _maxSuggestedVolumeSizePreview->intVal();
	bool simplePreview = isSimplePreview(brush, region);
	if (!simplePreview && canAllocatePreviewRegion(region, maxPreviewExtent)) {
		SelectBrush &selectBrush = modifier.selectBrush();
		const BrushType brushType = modifier.brushType();
		const bool isCircleSelect = brushType == BrushType::Select &&
			(selectBrush.selectMode() == SelectMode::Circle ||
			 selectBrush.selectMode() == SelectMode::Lasso);
		if (isCircleSelect) {
			selectBrush.setPreviewMode(true);
		}
		glm::ivec3 minsMirror = region.getLowerCorner();
		glm::ivec3 maxsMirror = region.getUpperCorner();
		if (brush->getMirrorAABB(minsMirror, maxsMirror)) {
			createOrClearPreviewVolume(existingVolume, _previewMirrorVolume, voxel::Region(minsMirror, maxsMirror));
			scenegraph::SceneGraphNode mirrorDummyNode(scenegraph::SceneGraphNodeType::Model);
			mirrorDummyNode.setUnownedVolume(_previewMirrorVolume);
			modifier.executeBrush(sceneGraph, mirrorDummyNode, modifierType, voxel);
		}
		createOrClearPreviewVolume(existingVolume, _previewVolume, region);
		scenegraph::SceneGraphNode dummyNode(scenegraph::SceneGraphNodeType::Model);
		dummyNode.setUnownedVolume(_previewVolume);
		modifier.executeBrush(sceneGraph, dummyNode, modifierType, voxel);
		if (isCircleSelect) {
			selectBrush.setPreviewMode(false);
		}
	} else if (simplePreview) {
		_brushPreview.useSimplePreview = true;
		_brushPreview.simplePreviewRegion = region;
		_brushPreview.simplePreviewColor = activePalette.color(brushContext.cursorVoxel.getColor());
		glm::ivec3 minsMirror = region.getLowerCorner();
		glm::ivec3 maxsMirror = region.getUpperCorner();
		if (brush->getMirrorAABB(minsMirror, maxsMirror)) {
			_brushPreview.simpleMirrorPreviewRegion = voxel::Region(minsMirror, maxsMirror);
		}
	}
}

} // namespace voxedit
