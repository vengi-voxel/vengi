/**
 * @file
 */

#include "Paint.h"
#include "scenegraph/SceneGraph.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxedit-util/modifier/brush/Brush.h"
#include "voxel/Connectivity.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxedit {
namespace select {

voxel::Region Paint::calcRegion(const BrushContext &ctx, const AABBBrushState &state) const {
	const int rad = state.radius;
	if (rad > 0) {
		return voxel::Region(ctx.cursorPosition - rad, ctx.cursorPosition + rad);
	}
	return voxel::Region(ctx.cursorPosition, ctx.cursorPosition);
}

bool Paint::beginBrush(const BrushContext &ctx, const AABBBrushState &state) {
	_accumulating = true;
	_hadSelection = false;
	_dirtyRegion = voxel::Region::InvalidRegion;
	_finalUndoRegion = voxel::Region::InvalidRegion;
	_modifiedFlags = SceneModifiedFlags::NoUndo;
	return false;
}

void Paint::endBrush(BrushContext &ctx) {
	if (_accumulating) {
		_accumulating = false;
		_finalUndoRegion = _dirtyRegion;
		_dirtyRegion = voxel::Region::InvalidRegion;
		_modifiedFlags = SceneModifiedFlags::All;
	}
}

void Paint::abort(BrushContext &ctx) {
	if (_accumulating) {
		_accumulating = false;
		_hadSelection = false;
		_dirtyRegion = voxel::Region::InvalidRegion;
		_modifiedFlags = SceneModifiedFlags::All;
	}
}

void Paint::reset() {
	_accumulating = false;
	_hadSelection = false;
	_dirtyRegion = voxel::Region::InvalidRegion;
	_finalUndoRegion = voxel::Region::InvalidRegion;
}

void Paint::generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
					 const voxel::Region &region, const AABBBrushState &state) {
	const glm::ivec3 center = ctx.cursorPosition;
	const int rad = state.radius;
	const int radSq = rad * rad;
	if (!_dirtyRegion.isValid() && !_hadSelection) {
		const int activeNodeId = sceneGraph.activeNode();
		if (sceneGraph.hasNode(activeNodeId)) {
			_hadSelection = sceneGraph.node(activeNodeId).hasSelection();
		}
	}
	const bool growOnly =
		_growRegion && wrapper.modifierType() != ModifierType::Erase && (_hadSelection || _dirtyRegion.isValid());
	auto func = [&wrapper](int x, int y, int z) {
		if (wrapper.modifierType() == ModifierType::Erase) {
			wrapper.removeFlagAt(x, y, z, voxel::FlagOutline);
		} else {
			wrapper.setFlagAt(x, y, z, voxel::FlagOutline);
		}
	};
	voxelutil::VisitSolid condition;
	auto paintFunc = [&](int x, int y, int z, const voxel::Voxel &voxel) {
		const int dx = x - center.x;
		const int dy = y - center.y;
		const int dz = z - center.z;
		if (dx * dx + dy * dy + dz * dz > radSq) {
			return;
		}
		if (growOnly) {
			bool hasSelectedNeighbor = false;
			for (const glm::ivec3 &off : ::voxel::arrayPathfinderFaces) {
				const glm::ivec3 npos(x + off.x, y + off.y, z + off.z);
				if (ctx.targetVolumeRegion.containsPoint(npos)) {
					const ::voxel::Voxel &neighborVoxel = wrapper.voxel(npos);
					if (!::voxel::isAir(neighborVoxel.getMaterial()) &&
						(neighborVoxel.getFlags() & ::voxel::FlagOutline)) {
						hasSelectedNeighbor = true;
						break;
					}
				}
			}
			if (!hasSelectedNeighbor) {
				return;
			}
		}
		func(x, y, z);
	};
	voxelutil::visitVolume(wrapper, region, paintFunc, condition);
	_dirtyRegion.accumulate(region);
}

voxel::Region Paint::consumeFinalUndoRegion() {
	voxel::Region region = _finalUndoRegion;
	_finalUndoRegion = voxel::Region::InvalidRegion;
	return region;
}

} // namespace select
} // namespace voxedit
