/**
 * @file
 */

#include "ExtrudeBrush.h"
#include "app/App.h"
#include "app/ForParallel.h"
#include "core/Common.h"
#include "math/Axis.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/Connectivity.h"
#include "voxel/DynamicVoxelArray.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeVisitor.h"
#include <glm/gtc/matrix_transform.hpp>

namespace voxedit {

void ExtrudeBrush::onSceneChange() {
	Super::onSceneChange();
	_active = false;
	_history.clear();
	_cachedSelBBoxValid = false;
}

void ExtrudeBrush::onActivated() {
	reset();
	_sceneModifiedFlags = SceneModifiedFlags::NoUndo;
}

bool ExtrudeBrush::hasPendingChanges() const {
	return _depth != 0 && _hasCachedSelection;
}

bool ExtrudeBrush::onDeactivated() {
	_sceneModifiedFlags = SceneModifiedFlags::All;
	return hasPendingChanges();
}

voxel::Region ExtrudeBrush::revertChanges(voxel::RawVolume *volume) {
	voxel::RawVolumeWrapper wrapper(volume);
	for (const voxel::VoxelPosition &entry : _history) {
		wrapper.setVoxel(entry.pos, entry.voxel);
	}
	_history.clear();
	return wrapper.dirtyRegion();
}

void ExtrudeBrush::reset() {
	Super::reset();
	_sceneModifiedFlags = SceneModifiedFlags::All;
	_depth = 0;
	_offsetU = 0;
	_offsetV = 0;
	_face = voxel::FaceNames::Max;
	_active = false;
	_history.clear();
	_cachedSelectedPositions.clear();
	_cachedWallCandidates.clear();
	_cachedSelectedBBox = voxel::Region::InvalidRegion;
	_hasCachedSelection = false;
	_capturedVolumeLower = glm::ivec3(0);
	_cachedSelBBoxValid = false;
	_fillWalls = true;
	_lastVolume = nullptr;
}

bool ExtrudeBrush::beginBrush(const BrushContext &ctx) {
	if (_active) {
		return false;
	}
	// Only update the stored face when the cursor is actually over a voxel face.
	// When triggered from the panel (cursorFace == Max), keep the last face from a viewport click.
	if (ctx.cursorFace != voxel::FaceNames::Max && !ctx.brushGizmoActive) {
		_face = ctx.cursorFace;
	}
	_active = true;
	return true;
}

void ExtrudeBrush::endBrush(BrushContext &) {
	// Prune interior voxels after final commit - voxels fully enclosed by 6 solid
	// neighbors are invisible and waste sparse storage.
	if (_lastVolume && _depth > 0) {
		const voxel::Region &volRegion = _lastVolume->region();
		const voxel::Voxel air;
		auto isInterior = [&](const glm::ivec3 &pos) {
			for (int ni = 0; ni < lengthof(voxel::arrayPathfinderFaces); ++ni) {
				const glm::ivec3 nb = pos + voxel::arrayPathfinderFaces[ni];
				if (!volRegion.containsPoint(nb) || voxel::isAir(_lastVolume->voxel(nb).getMaterial())) {
					return false;
				}
			}
			return true;
		};

		core::DynamicArray<glm::ivec3> toPrune;
		toPrune.reserve(_history.size() + _cachedSelectedPositions.size());
		for (const voxel::VoxelPosition &entry : _history) {
			if (!voxel::isAir(_lastVolume->voxel(entry.pos).getMaterial()) && isInterior(entry.pos)) {
				toPrune.push_back(entry.pos);
			}
		}
		for (const glm::ivec3 &selPos : _cachedSelectedPositions) {
			if (!voxel::isAir(_lastVolume->voxel(selPos).getMaterial()) && isInterior(selPos)) {
				toPrune.push_back(selPos);
			}
		}
		for (const glm::ivec3 &pos : toPrune) {
			_lastVolume->setVoxel(pos, air);
		}
	}
	_lastVolume = nullptr;
	_active = false;
	// _face is intentionally kept: the user sets it by clicking a voxel face in the viewport,
	// then adjusts depth via the panel without needing to hover the viewport again.
}

void ExtrudeBrush::preExecute(const BrushContext &ctx, const voxel::RawVolume *volume) {
	Super::preExecute(ctx, volume);
	_cachedSelBBoxValid = false;
	if (!volume) {
		return;
	}

	const voxel::Region &volRegion = volume->region();

	// Detect volume region shift and adjust cached data
	if (_hasCachedSelection) {
		const glm::ivec3 delta = volRegion.getLowerCorner() - _capturedVolumeLower;
		if (delta != glm::ivec3(0)) {
			adjustCacheForRegionShift(delta);
		}
	}

	// Build the full selection cache (positions + wall candidates) once when
	// the face is set and no cache exists yet. Subsequent frames reuse it.
	if (!_hasCachedSelection && _face != voxel::FaceNames::Max) {
		cacheSelection(volume, volRegion);
	}

	// Use cached bbox for calcRegion() if available
	if (_hasCachedSelection) {
		_cachedSelBBox = _cachedSelectedBBox;
		_cachedSelBBoxValid = true;
	}
}

bool ExtrudeBrush::active() const {
	return _active || _cachedSelBBoxValid;
}

voxel::Region ExtrudeBrush::calcRegion(const BrushContext &ctx) const {
	if (!_cachedSelBBoxValid || _face == voxel::FaceNames::Max) {
		return ctx.targetVolumeRegion;
	}
	// Expand the selection bbox along the extrusion direction to cover the
	// extruded/carved voxels, plus lateral offsets and a small side-wall margin
	// on perpendicular axes. Only expand in the direction that is actually affected
	// to keep the region tight (large regions exceed maxPreviewRegion).
	const math::Axis axis = voxel::faceToAxis(_face);
	const int axisIdx = math::getIndexForAxis(axis);
	const int faceSign = voxel::isNegativeFace(_face) ? -1 : 1;
	const bool carving = _depth < 0;
	const int dirSign = carving ? -faceSign : faceSign;
	const int steps = glm::abs(_depth);

	static constexpr int NumAxes = 3;
	const int perp1 = (axisIdx + 1) % NumAxes;
	const int perp2 = (axisIdx + 2) % NumAxes;

	glm::ivec3 lo = _cachedSelBBox.getLowerCorner();
	glm::ivec3 hi = _cachedSelBBox.getUpperCorner();

	// Extend along extrusion axis by depth steps
	if (dirSign > 0) {
		hi[axisIdx] += steps;
	} else {
		lo[axisIdx] -= steps;
	}

	// Lateral offset shifts
	if (_offsetU > 0) {
		hi[perp1] += _offsetU;
	} else if (_offsetU < 0) {
		lo[perp1] += _offsetU;
	}
	if (_offsetV > 0) {
		hi[perp2] += _offsetV;
	} else if (_offsetV < 0) {
		lo[perp2] += _offsetV;
	}

	// Small margin for side walls on perpendicular axes
	static constexpr int SideWallMargin = 1;
	lo[perp1] -= SideWallMargin;
	hi[perp1] += SideWallMargin;
	lo[perp2] -= SideWallMargin;
	hi[perp2] += SideWallMargin;

	voxel::Region region(lo, hi);
	region.cropTo(ctx.targetVolumeRegion);
	return region;
}

void ExtrudeBrush::setDepth(int depth) {
	if (_depth == depth) {
		return;
	}
	_depth = depth;
	markDirty();
}

void ExtrudeBrush::setOffsetU(int offset) {
	if (_offsetU == offset) {
		return;
	}
	_offsetU = offset;
	markDirty();
}

void ExtrudeBrush::setOffsetV(int offset) {
	if (_offsetV == offset) {
		return;
	}
	_offsetV = offset;
	markDirty();
}

void ExtrudeBrush::generate(scenegraph::SceneGraph &, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
							const voxel::Region &) {
	_lastVolume = wrapper.volume();
	evaluate(wrapper, ctx.cursorVoxel);
}

bool ExtrudeBrush::wantBrushGizmo(const BrushContext &ctx) const {
	return _cachedSelBBoxValid && _face != voxel::FaceNames::Max;
}

void ExtrudeBrush::brushGizmoState(const BrushContext &ctx, BrushGizmoState &state) const {
	if (!_cachedSelBBoxValid || _face == voxel::FaceNames::Max) {
		state.operations = BrushGizmo_None;
		return;
	}

	const glm::vec3 center = glm::vec3(_cachedSelBBox.getCenter());
	const glm::vec3 normal = voxel::faceNormal(_face);

	// Position the gizmo at the selection center, offset by current depth along the face normal
	state.matrix = glm::translate(glm::mat4(1.0f), center + normal * (float)_depth);

	// Only allow translation along the extrusion axis
	const math::Axis axis = voxel::faceToAxis(_face);
	switch (axis) {
	case math::Axis::X:
		state.operations = BrushGizmo_TranslateX;
		break;
	case math::Axis::Y:
		state.operations = BrushGizmo_TranslateY;
		break;
	case math::Axis::Z:
		state.operations = BrushGizmo_TranslateZ;
		break;
	default:
		state.operations = BrushGizmo_None;
		return;
	}

	state.snap[0] = state.snap[1] = state.snap[2] = 1.0f;
	state.localMode = false;
}

bool ExtrudeBrush::applyBrushGizmo(BrushContext &ctx, const glm::mat4 &matrix,
								   const glm::mat4 &deltaMatrix, uint32_t operation) {
	if (_face == voxel::FaceNames::Max) {
		return false;
	}

	const math::Axis axis = voxel::faceToAxis(_face);
	const int axisIdx = math::getIndexForAxis(axis);
	const glm::vec3 translation(deltaMatrix[3]);
	const int delta = (int)glm::round(translation[axisIdx]);
	if (delta == 0) {
		return false;
	}

	// Convert the axis translation to a signed depth change
	const int faceSign = voxel::isNegativeFace(_face) ? -1 : 1;
	setDepth(_depth + delta * faceSign);
	return true;
}

} // namespace voxedit
