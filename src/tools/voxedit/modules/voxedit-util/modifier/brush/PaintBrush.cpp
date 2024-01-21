/**
 * @file
 */

#include "PaintBrush.h"
#include "core/Color.h"
#include "palette/Palette.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeVisitor.h"
#include "voxelutil/VoxelUtil.h"

namespace voxedit {

voxel::Voxel PaintBrush::VoxelColor::evaluate(const voxel::Voxel &old) const {
	if (_paintMode == PaintMode::Replace) {
		return _voxel;
	}
	const core::RGBA voxelColor = _palette.color(old.getColor());
	core::RGBA newColor;
	if (_paintMode == PaintMode::Brighten) {
		newColor = core::Color::brighter(voxelColor);
	} else {
		newColor = core::Color::darker(voxelColor);
	}
	int index = _palette.getClosestMatch(newColor);
	if (index == palette::PaletteColorNotFound) {
		return old;
	}
	return voxel::Voxel(old.getMaterial(), index, old.getFlags());
}

bool PaintBrush::generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper,
						  const BrushContext &context, const voxel::Region &region) {
	if (_plane) {
		voxelutil::paintPlane(wrapper, context.cursorPosition, context.cursorFace, context.hitCursorVoxel,
							  context.cursorVoxel);
	} else {
		const VoxelColor voxelColor(wrapper.node().palette(), context.cursorVoxel, _paintMode);
		auto visitor = [&](int x, int y, int z, const voxel::Voxel &voxel) {
			wrapper.setVoxel(x, y, z, voxelColor.evaluate(voxel));
		};
		voxelutil::visitVolume(wrapper, region, visitor, voxelutil::SkipEmpty());
	}

	return true;
}

} // namespace voxedit
