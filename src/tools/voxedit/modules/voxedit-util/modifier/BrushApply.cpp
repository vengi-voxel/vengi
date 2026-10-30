/**
 * @file
 */

#include "BrushApply.h"
#include "ModifierVolumeWrapper.h"
#include "core/ScopedPtr.h"
#include "palette/NormalPalette.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/RawVolume.h"
#include <glm/common.hpp>

namespace voxedit {

/**
 * change the cursor position if the brush region is outside the volume
 * this allows us to keep all voxels inside the volume boundaries even on the
 * +x, +y and +z sides where the voxels are currently flowing out of the volume
 */
glm::ivec3 clampBrushCursor(const voxel::Region &volumeRegion, const voxel::Region &brushRegion,
							const glm::ivec3 &cursor) {
	if (!brushRegion.isValid()) {
		return cursor;
	}
	if (volumeRegion.containsRegion(brushRegion)) {
		return cursor;
	}
	glm::ivec3 delta(0);
	if (brushRegion.getUpperX() > volumeRegion.getUpperX()) {
		delta.x = volumeRegion.getUpperX() - brushRegion.getUpperX();
	}
	if (brushRegion.getUpperY() > volumeRegion.getUpperY()) {
		delta.y = volumeRegion.getUpperY() - brushRegion.getUpperY();
	}
	if (brushRegion.getUpperZ() > volumeRegion.getUpperZ()) {
		delta.z = volumeRegion.getUpperZ() - brushRegion.getUpperZ();
	}

	if (brushRegion.getLowerX() < volumeRegion.getLowerX()) {
		delta.x = glm::abs(volumeRegion.getLowerX() - brushRegion.getUpperX());
	}
	if (brushRegion.getLowerY() < volumeRegion.getLowerY()) {
		delta.y = glm::abs(volumeRegion.getLowerY() - brushRegion.getLowerY());
	}
	if (brushRegion.getLowerZ() < volumeRegion.getLowerZ()) {
		delta.z = glm::abs(volumeRegion.getLowerZ() - brushRegion.getLowerZ());
	}

	return cursor + delta;
}

static BrushContext makeEvalContext(const BrushSample &sample, const BrushContext &settings, bool preview) {
	BrushContext ctx = settings;
	ctx.cursorPosition = sample.position;
	ctx.prevCursorPosition = sample.position;
	ctx.cursorFace = sample.face;
	ctx.modifierType = sample.operation;
	ctx.lockedAxis = sample.lockedAxis;
	ctx.gridResolution = sample.gridResolution;
	ctx.cursorVoxel = sample.cursorVoxel;
	ctx.hitCursorVoxel = sample.hitCursorVoxel;
	ctx.voxelAtCursor = sample.voxelAtCursor;
	ctx.fixedOrthoSideView = sample.fixedOrthoSideView;
	ctx.normalPaint = sample.normalPaint;
	ctx.normalIndex = sample.normalIndex;
	ctx.preview = preview;
	ctx.brushGizmoActive = false;
	if (sample.operation == ModifierType::NormalPaint) {
		ctx.cursorVoxel.setNormal(sample.normalIndex + NORMAL_PALETTE_OFFSET);
	}
	return ctx;
}

static BrushApplyResult invalidTargetResult() {
	BrushApplyResult result;
	result.status = BrushApplyStatus::InvalidTarget;
	return result;
}

BrushApplyResult applyBrushSample(Brush &brush, scenegraph::SceneGraph &sceneGraph, const BrushSample &sample,
								  BrushApplyTarget &target, const BrushContext &settings,
								  const BrushApplyOptions &options) {
	if (target.node == nullptr || target.volume == nullptr) {
		return invalidTargetResult();
	}
	if (target.node->volume() != target.volume) {
		return invalidTargetResult();
	}
	voxel::RawVolume *writeVolume = target.writeVolume != nullptr ? target.writeVolume : target.volume;
	if (writeVolume == nullptr) {
		return invalidTargetResult();
	}

	voxel::RawVolume *destination = writeVolume;
	core::ScopedPtr<voxel::RawVolume> slice;
	if (target.sliceRegion.isValid()) {
		voxel::Region region = writeVolume->region();
		if (!region.cropTo(target.sliceRegion)) {
			return {};
		}
		if (region != writeVolume->region()) {
			slice = new voxel::RawVolume(*writeVolume, region);
			writeVolume = slice;
		}
	}

	BrushContext ctx = makeEvalContext(sample, settings, options.preview);
	ctx.targetVolumeRegion = target.bounds.isValid() ? target.bounds : writeVolume->region();
	if (brush.clampToVolume()) {
		const voxel::Region brushRegion = brush.calcRegion(ctx);
		ctx.cursorPosition = clampBrushCursor(ctx.targetVolumeRegion, brushRegion, sample.position);
	}
	if (target.sliceRegion.isValid()) {
		ctx.cursorPosition =
			glm::clamp(ctx.cursorPosition, target.sliceRegion.getLowerCorner(), target.sliceRegion.getUpperCorner());
	}

	brush.preExecute(ctx, writeVolume);

	const ModifierType volumeModifierType = brush.volumeModifier(sample.operation);
	ModifierVolumeWrapper wrapper(*target.node, volumeModifierType, target.selectionRegion);
	if (writeVolume != target.node->volume()) {
		wrapper.setVolume(writeVolume);
	}

	const uint64_t paletteHash = target.node->palette().hash();
	const uint32_t normalPaletteHash = target.node->normalPalette().hash();
	const bool ok = brush.executeSample(sceneGraph, wrapper, ctx);
	if (options.applySelectionPolicy && !options.preview) {
		const bool isPlacementBrush = brush.autoSelectPlacement();
		const bool isPlacementModifier = sample.operation == ModifierType::Place;
		if (options.autoSelect && isPlacementBrush && isPlacementModifier) {
			wrapper.autoSelectNewVoxels();
		} else if (!brush.managesOwnSelection()) {
			wrapper.growSelectionToNewVoxels();
		}
	}

	BrushApplyResult result;
	result.dirtyRegion = wrapper.dirtyRegion();
	result.paletteChanged = paletteHash != target.node->palette().hash();
	result.normalPaletteChanged = normalPaletteHash != target.node->normalPalette().hash();
	if (target.node->volume() != target.volume) {
		result.status = BrushApplyStatus::Failed;
		return result;
	}
	if (slice && result.dirtyRegion.isValid()) {
		destination->copyInto(*slice, result.dirtyRegion);
	}
	if (!ok) {
		result.status = BrushApplyStatus::Failed;
		return result;
	}
	result.status = result.changed() ? BrushApplyStatus::Changed : BrushApplyStatus::Unchanged;
	return result;
}

} // namespace voxedit
