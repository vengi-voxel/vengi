/**
 * @file
 */

#include "ExtrudeBrush.h"
#include "math/Axis.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/Connectivity.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
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
}

bool ExtrudeBrush::onDeactivated() {
	return _depth != 0 && _hasCachedSelection;
}

void ExtrudeBrush::reset() {
	Super::reset();
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
	if (ctx.cursorFace != voxel::FaceNames::Max) {
		_face = ctx.cursorFace;
	}
	_active = true;
	return true;
}

void ExtrudeBrush::endBrush(BrushContext &) {
	// Prune interior voxels after final commit — voxels fully enclosed by 6 solid
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
		for (const HistoryEntry &entry : _history) {
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

void ExtrudeBrush::cacheSelection(const voxel::RawVolume *vol, const voxel::Region &volRegion) {
	_cachedSelectedPositions.clear();
	_cachedWallCandidates.clear();
	_hasCachedSelection = false;

	if (_face == voxel::FaceNames::Max) {
		return;
	}

	// Collect selected (FlagOutline) voxel positions
	glm::ivec3 selLo(volRegion.getUpperCorner());
	glm::ivec3 selHi(volRegion.getLowerCorner());
	PositionSet selectedSet;

	const glm::ivec3 &vlo = volRegion.getLowerCorner();
	const glm::ivec3 &vhi = volRegion.getUpperCorner();
	// TODO: PERF: use volume visitor
	for (int z = vlo.z; z <= vhi.z; ++z) {
		for (int y = vlo.y; y <= vhi.y; ++y) {
			for (int x = vlo.x; x <= vhi.x; ++x) {
				const voxel::Voxel vx = vol->voxel(x, y, z);
				if (!voxel::isAir(vx.getMaterial()) && (vx.getFlags() & voxel::FlagOutline)) {
					const glm::ivec3 pos(x, y, z);
					_cachedSelectedPositions.push_back(pos);
					selectedSet.insert(pos);
					selLo = glm::min(selLo, pos);
					selHi = glm::max(selHi, pos);
				}
			}
		}
	}

	if (_cachedSelectedPositions.empty()) {
		return;
	}

	_cachedSelectedBBox = voxel::Region(selLo, selHi);

	// Compute wall candidates: for each selected voxel, check 4 perpendicular neighbors.
	// A wall candidate is a (selectedPos, perpOffset) pair where the perpendicular neighbor
	// is solid and not selected — indicating an edge that needs sealing during extrusion.
	const math::Axis axis = voxel::faceToAxis(_face);
	const int axisIdx = math::getIndexForAxis(axis);
	static constexpr int NumAxes = 3;
	const int perp1 = (axisIdx + 1) % NumAxes;
	const int perp2 = (axisIdx + 2) % NumAxes;

	static constexpr int NumPerpOffsets = 4;
	glm::ivec3 perpOffsets[NumPerpOffsets] = {};
	perpOffsets[0][perp1] = 1;
	perpOffsets[1][perp1] = -1;
	perpOffsets[2][perp2] = 1;
	perpOffsets[3][perp2] = -1;

	for (const glm::ivec3 &selPos : _cachedSelectedPositions) {
		for (int pi = 0; pi < NumPerpOffsets; ++pi) {
			const glm::ivec3 neighborPos = selPos + perpOffsets[pi];
			if (!volRegion.containsPoint(neighborPos)) {
				continue;
			}
			const voxel::Voxel &nv = vol->voxel(neighborPos);
			if (voxel::isAir(nv.getMaterial())) {
				continue;
			}
			if (selectedSet.has(neighborPos)) {
				continue;
			}
			_cachedWallCandidates.push_back({selPos, perpOffsets[pi]});
		}
	}

	_capturedVolumeLower = volRegion.getLowerCorner();
	_hasCachedSelection = true;
}

void ExtrudeBrush::adjustCacheForRegionShift(const glm::ivec3 &delta) {
	for (glm::ivec3 &pos : _cachedSelectedPositions) {
		pos += delta;
	}
	_cachedSelectedBBox.shift(delta.x, delta.y, delta.z);

	for (WallCandidate &wc : _cachedWallCandidates) {
		wc.basePos += delta;
	}

	for (HistoryEntry &entry : _history) {
		entry.pos += delta;
	}

	_capturedVolumeLower += delta;
}

void ExtrudeBrush::writeVoxel(ModifierVolumeWrapper &wrapper, PositionSet &savedPositions, const glm::ivec3 &pos, const voxel::Voxel &newVoxel) {
	voxel::RawVolume *vol = wrapper.volume();
	const voxel::Region &volRegion = vol->region();
	if (!volRegion.containsPoint(pos)) {
		return;
	}
	if (!savedPositions.has(pos)) {
		_history.push_back({pos, vol->voxel(pos)});
		savedPositions.insert(pos);
	}
	vol->setVoxel(pos, newVoxel);
	wrapper.addToDirtyRegion(pos);
}

void ExtrudeBrush::generate(scenegraph::SceneGraph &, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
							const voxel::Region &) {
	voxel::RawVolume *vol = wrapper.volume();
	_lastVolume = vol;

	// Step 1: Restore all positions modified by the previous generate() call.
	// This makes depth/offset changes fully reversible without touching the undo stack.
	for (const HistoryEntry &entry : _history) {
		vol->setVoxel(entry.pos, entry.original);
		wrapper.addToDirtyRegion(entry.pos);
	}
	_history.clear();

	if (_depth == 0 || _face == voxel::FaceNames::Max || !_hasCachedSelection) {
		return;
	}

	const math::Axis axis = voxel::faceToAxis(_face);
	const int axisIdx = math::getIndexForAxis(axis);
	const int faceSign = voxel::isNegativeFace(_face) ? -1 : 1;

	const bool carving = _depth < 0;
	const int steps = glm::abs(_depth);
	const int dirSign = carving ? -faceSign : faceSign;

	glm::ivec3 dir(0);
	dir[axisIdx] = dirSign;

	static constexpr int NumAxes = 3;
	const int perp1 = (axisIdx + 1) % NumAxes;
	const int perp2 = (axisIdx + 2) % NumAxes;

	const float stepU = static_cast<float>(_offsetU) / steps;
	const float stepV = static_cast<float>(_offsetV) / steps;
	auto lateralShift = [&](int step) -> glm::ivec3 {
		glm::ivec3 shift(0);
		if (_offsetU != 0) {
			shift[perp1] = static_cast<int>(stepU * step);
		}
		if (_offsetV != 0) {
			shift[perp2] = static_cast<int>(stepV * step);
		}
		return shift;
	};

	// Step 2: Save original voxel before writing - uses a set for O(1) dedup.
	// Saves both air and solid voxels so restore is always complete.
	PositionSet savedPositions;

	const voxel::Voxel air{};

	// Step 3: Apply extrusion or carving
	if (carving) {
		// Loop 1: Place walls at each depth step before carving
		if (_fillWalls) {
			for (int step = 0; step < steps; ++step) {
				const glm::ivec3 shift = lateralShift(step);
				for (const WallCandidate &wc : _cachedWallCandidates) {
					const glm::ivec3 sidePos = wc.basePos + wc.perpOffset + dir * step + shift;
					writeVoxel(wrapper, savedPositions, sidePos, ctx.cursorVoxel);
				}
			}
		}
		// Loop 2: Carve out cached selected positions at each depth step
		for (int step = 0; step < steps; ++step) {
			const glm::ivec3 shift = lateralShift(step);
			for (const glm::ivec3 &selPos : _cachedSelectedPositions) {
				writeVoxel(wrapper, savedPositions, selPos + dir * step + shift, air);
			}
		}
	} else {
		// Positive depth: place selected voxels at each step from 1 to depth
		for (int step = 1; step <= steps; ++step) {
			const glm::ivec3 shift = lateralShift(step);
			for (const glm::ivec3 &selPos : _cachedSelectedPositions) {
				writeVoxel(wrapper, savedPositions, selPos + dir * step + shift, ctx.cursorVoxel);
			}
		}

		// Remove original selected voxels — extrude moves the face outward
		for (const glm::ivec3 &selPos : _cachedSelectedPositions) {
			writeVoxel(wrapper, savedPositions, selPos, air);
		}
	}
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

	state.snap = 1.0f;
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
