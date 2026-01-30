/**
 * @file
 */

#include "SelectBrush.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeVisitor.h"
#include "palette/Palette.h"
#include <glm/geometric.hpp>

namespace voxedit {

voxel::Region SelectBrush::calcRegion(const BrushContext &ctx) const {
	if (_selectMode == SelectMode::Connected || _selectMode == SelectMode::SameColor ||
		_selectMode == SelectMode::Surface || _selectMode == SelectMode::FuzzyColor) {
		return ctx.targetVolumeRegion;
	}
	return Super::calcRegion(ctx);
}

void SelectBrush::generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
						   const voxel::Region &region) {
	voxel::Region selectionRegion = region;
	if (_brushClamping) {
		selectionRegion.cropTo(ctx.targetVolumeRegion);
	}

	auto func = [&wrapper](int x, int y, int z, const voxel::Voxel &voxel) {
		if (wrapper.modifierType() == ModifierType::Erase) {
			wrapper.removeFlagAt(x, y, z, voxel::FlagOutline);
		} else {
			wrapper.setFlagAt(x, y, z, voxel::FlagOutline);
		}
	};

	switch (_selectMode) {
	case SelectMode::All: {
		voxelutil::VisitVisible condition = voxelutil::VisitVisible();
		voxelutil::visitVolumeParallel(wrapper, selectionRegion, func, condition);
		break;
	}
	case SelectMode::Surface: {
		voxelutil::visitSurfaceVolumeParallel(wrapper, func);
		break;
	}
	case SelectMode::SameColor: {
		const voxel::Voxel &referenceVoxel = ctx.hitCursorVoxel;
		if (voxel::isAir(referenceVoxel.getMaterial())) {
			return;
		}
		voxelutil::VisitVoxelColor condition = voxelutil::VisitVoxelColor(referenceVoxel);
		voxelutil::visitVolumeParallel(wrapper, selectionRegion, func, condition);
		break;
	}
	case SelectMode::FuzzyColor: {
		const voxel::Voxel &referenceVoxel = ctx.hitCursorVoxel;
		if (voxel::isAir(referenceVoxel.getMaterial())) {
			return;
		}
		const palette::Palette &palette = wrapper.node().palette();
		voxelutil::VisitVoxelFuzzyColor condition(palette, referenceVoxel.getColor(), _colorThreshold);
		voxelutil::visitVolumeParallel(wrapper, selectionRegion, func, condition);
		break;
	}
	case SelectMode::Connected: {
		const voxel::Voxel &referenceVoxel = ctx.hitCursorVoxel;
		if (voxel::isAir(referenceVoxel.getMaterial())) {
			return;
		}
		const glm::ivec3 &startPos = ctx.cursorPosition;
		if (wrapper.modifierType() == ModifierType::Erase) {
			wrapper.removeFlagAt(startPos.x, startPos.y, startPos.z, voxel::FlagOutline);
		} else {
			wrapper.setFlagAt(startPos.x, startPos.y, startPos.z, voxel::FlagOutline);
		}
		voxelutil::visitConnectedByCondition(wrapper, startPos, func);
		break;
	}
	case SelectMode::Max:
		return;
	}
}

} // namespace voxedit
