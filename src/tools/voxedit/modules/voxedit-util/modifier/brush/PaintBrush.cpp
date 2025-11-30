/**
 * @file
 */

#include "PaintBrush.h"
#include "color/Color.h"
#include "palette/Palette.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeVisitor.h"
#include "voxelutil/VoxelUtil.h"

namespace voxedit {

voxel::Voxel PaintBrush::VoxelColor::evaluate(const voxel::Voxel &old) {
	if (_paintMode == PaintMode::Replace) {
		return _voxel;
	}
	if (_paintMode == PaintMode::Random) {
		int n = _palette.colorCount();
		int idx = rand() % n;
		return voxel::createVoxel(_palette, idx, old.getFlags());
	}

	bool brighten = _paintMode == PaintMode::Brighten;
	if (_paintMode == PaintMode::Variation) {
		if (rand() % _variationThreshold != 0) {
			return old;
		}
		brighten = rand() % 2 == 0;
	}

	const color::RGBA voxelColor = _palette.color(old.getColor());
	color::RGBA newColor;
	if (brighten) {
		newColor = color::brighter(voxelColor, _factor);
	} else {
		newColor = color::darker(voxelColor, _factor);
	}
	const int index = _palette.getClosestMatch(newColor, old.getColor());
	if (index == palette::PaletteColorNotFound) {
		uint8_t newColorIndex = 0;
		if (!_palette.tryAdd(newColor, false, &newColorIndex, false, old.getColor())) {
			return old;
		}
		_palette.markDirty();
		_palette.markSave();
		// TODO: MEMENTO: no memento state handling for the palette here
		return voxel::createVoxel(_palette, newColorIndex, old.getFlags());
	}
	return voxel::createVoxel(_palette, index, old.getFlags());
}

static voxel::Voxel mix(ModifierVolumeWrapper &wrapper, const voxel::Voxel &from, const voxel::Voxel &to,
						float factor) {
	const palette::Palette &palette = wrapper.node().palette();
	const glm::vec4 colorA = palette.color4(from.getColor());
	const glm::vec4 colorB = palette.color4(to.getColor());
	const glm::vec4 newColor = glm::mix(colorA, colorB, factor);
	const int index = palette.getClosestMatch(color::getRGBA(newColor), from.getColor());
	if (index == palette::PaletteColorNotFound) {
		return from;
	}
	return voxel::createVoxel(palette, index);
}

void PaintBrush::generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
						  const voxel::Region &region) {
	VoxelColor voxelColor(wrapper.node().palette(), ctx.cursorVoxel, _paintMode, _factor, _variationThreshold);
	if (plane()) {
		voxelutil::paintPlane(wrapper, region.getLowerCorner(), ctx.cursorFace, ctx.hitCursorVoxel,
							  voxelColor.evaluate(ctx.hitCursorVoxel));
	} else if (gradient()) {
		const glm::ivec3 start = ctx.cursorPosition;
		const glm::ivec3 size = region.getDimensionsInVoxels();
		auto func = [&](int x, int y, int z, const voxel::Voxel &voxel) {
			const float factor = glm::distance(glm::vec3(x, y, z), glm::vec3(start)) / glm::length(glm::vec3(size));
			const voxel::Voxel evalVoxel = voxelColor.evaluate(voxel);
			const voxel::Voxel newVoxel = mix(wrapper, ctx.hitCursorVoxel, evalVoxel, factor);
			wrapper.setVoxel(x, y, z, newVoxel);
		};
		voxelutil::visitVolumeParallel(wrapper, region, func);
	} else {
		auto func = [&](int x, int y, int z, const voxel::Voxel &voxel) {
			wrapper.setVoxel(x, y, z, voxelColor.evaluate(voxel));
		};
		voxelutil::visitVolumeParallel(wrapper, region, func);
	}
}

bool PaintBrush::wantAABB() const {
	if (plane()) {
		return false;
	}
	return Super::wantAABB();
}

void PaintBrush::setFactor(float factor) {
	_factor = glm::clamp(factor, 0.1f, 10.0f);
	markDirty();
}

void PaintBrush::setVariationThreshold(int variationThreshold) {
	_variationThreshold = glm::clamp(variationThreshold, 2, 20);
	markDirty();
}

} // namespace voxedit
