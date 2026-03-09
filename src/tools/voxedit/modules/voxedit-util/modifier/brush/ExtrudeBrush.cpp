/**
 * @file
 */

#include "ExtrudeBrush.h"
#include "math/Axis.h"
#include "voxedit-util/modifier/ModifierType.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/Connectivity.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include <glm/gtc/matrix_transform.hpp>

namespace voxedit {

void ExtrudeBrush::reset() {
	Super::reset();
	_depth = 0;
	_offsetU = 0;
	_offsetV = 0;
	_face = voxel::FaceNames::Max;
	_active = false;
	_history.clear();
	_cachedSelBBoxValid = false;
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
	// Scan for selected (FlagOutline) voxels and cache their bounding box.
	// This is used by calcRegion() to return a tight region for preview.
	const voxel::Region &volRegion = volume->region();
	glm::ivec3 selLo(volRegion.getUpperCorner());
	glm::ivec3 selHi(volRegion.getLowerCorner());
	const glm::ivec3 &vlo = volRegion.getLowerCorner();
	const glm::ivec3 &vhi = volRegion.getUpperCorner();
	for (int z = vlo.z; z <= vhi.z; ++z) {
		for (int y = vlo.y; y <= vhi.y; ++y) {
			for (int x = vlo.x; x <= vhi.x; ++x) {
				const voxel::Voxel vx = volume->voxel(x, y, z);
				if (!voxel::isAir(vx.getMaterial()) && (vx.getFlags() & voxel::FlagOutline)) {
					selLo = glm::min(selLo, glm::ivec3(x, y, z));
					selHi = glm::max(selHi, glm::ivec3(x, y, z));
				}
			}
		}
	}
	if (selLo.x <= selHi.x) {
		_cachedSelBBox = voxel::Region(selLo, selHi);
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
	voxel::RawVolume *vol = wrapper.volume();
	const voxel::Region &volRegion = vol->region();

	// Restore all positions from a previous extrude in this session before re-applying.
	// This makes depth changes fully reversible without touching the undo stack.
	for (const HistoryEntry &entry : _history) {
		if (vol->setVoxel(entry.pos, entry.original)) {
			wrapper.addToDirtyRegion(entry.pos);
		}
	}
	_history.clear();

	if (_depth == 0 || _face == voxel::FaceNames::Max) {
		return;
	}

	const math::Axis axis = voxel::faceToAxis(_face);
	const int axisIdx = math::getIndexForAxis(axis);
	const int faceSign = voxel::isNegativeFace(_face) ? -1 : 1;

	// Positive depth -> extrude outward (along face normal, place voxels)
	// Negative depth -> carve inward (opposite to face normal, erase voxels)
	const bool carving = _depth < 0;
	const int steps = glm::abs(_depth);
	const int dirSign = carving ? -faceSign : faceSign;

	glm::ivec3 dir(0);
	dir[axisIdx] = dirSign;

	// Perpendicular axes used for lateral offset (sweep) and side-wall filling.
	static constexpr int NumAxes = 3;
	const int perp1 = (axisIdx + 1) % NumAxes;
	const int perp2 = (axisIdx + 2) % NumAxes;

	// Compute lateral shift at a given extrusion step via linear interpolation.
	// shift = offset * step / depth (truncated toward zero).
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

	// Save original voxel at pos (only once per session) then write the new voxel.
	auto writeVoxel = [&](const glm::ivec3 &pos, const voxel::Voxel &newVoxel) {
		if (!volRegion.containsPoint(pos)) {
			return;
		}
		// Linear search - history is small enough that this is fine.
		bool alreadySaved = false;
		for (const HistoryEntry &entry : _history) {
			if (entry.pos == pos) {
				alreadySaved = true;
				break;
			}
		}
		if (!alreadySaved) {
			_history.push_back({pos, vol->voxel(pos)});
		}
		if (vol->setVoxel(pos, newVoxel)) {
			wrapper.addToDirtyRegion(pos);
		}
	};

	const voxel::Voxel air{};

	// Collect selected (FlagOutline) voxel positions to avoid scanning the full volume
	// (which can be 256^3+) in the extrusion and fill-sides loops.
	core::DynamicArray<glm::ivec3> selectedPositions;
	glm::ivec3 selLo(volRegion.getUpperCorner());
	glm::ivec3 selHi(volRegion.getLowerCorner());
	{
		const glm::ivec3 &vlo = volRegion.getLowerCorner();
		const glm::ivec3 &vhi = volRegion.getUpperCorner();
		for (int z = vlo.z; z <= vhi.z; ++z) {
			for (int y = vlo.y; y <= vhi.y; ++y) {
				for (int x = vlo.x; x <= vhi.x; ++x) {
					const voxel::Voxel vx = vol->voxel(x, y, z);
					if (!voxel::isAir(vx.getMaterial()) && (vx.getFlags() & voxel::FlagOutline)) {
						const glm::ivec3 pos(x, y, z);
						selectedPositions.push_back(pos);
						selLo = glm::min(selLo, pos);
						selHi = glm::max(selHi, pos);
					}
				}
			}
		}
	}
	if (selectedPositions.empty()) {
		return; // no selected voxels
	}
	const glm::ivec3 &lo = selLo;
	const glm::ivec3 &hi = selHi;

	// Carve or extrude each selected voxel along dir.
	for (const glm::ivec3 &selPos : selectedPositions) {
		// Carving starts at the selected voxel itself (step=0) so the surface layer
		// becomes air. Extrusion starts at step=1 to place new voxels outward.
		const int stepFirst = carving ? 0 : 1;
		const int stepLast = carving ? steps - 1 : steps;
		for (int step = stepFirst; step <= stepLast; ++step) {
			const glm::ivec3 shift = lateralShift(step);
			const glm::ivec3 newPos = selPos + dir * step + shift;
			if (!volRegion.containsPoint(newPos)) {
				break;
			}
			writeVoxel(newPos, carving ? air : ctx.cursorVoxel);
		}
	}

	// Expanded bounding box covering extruded voxels after lateral offset + side wall margin.
	// The fill-sides and pruning loops need to find selected voxels which may have been
	// shifted outside the original selection bbox (lo..hi) by the lateral offset.
	const glm::ivec3 tipShift = lateralShift(steps);
	const glm::ivec3 extLo = glm::max(glm::min(lo, lo + dir * steps + tipShift) - glm::ivec3(1),
									   volRegion.getLowerCorner());
	const glm::ivec3 extHi = glm::min(glm::max(hi, hi + dir * steps + tipShift) + glm::ivec3(1),
									   volRegion.getUpperCorner());

	// Fill perpendicular side walls - only meaningful when extruding straight outward
	// (no lateral offset). With lateral offsets the extrusion is a sweep/shear and
	// wall placement doesn't make geometric sense.
	// Side walls are only placed when the neighbor already has pre-existing solid
	// material extending in the extrusion direction (e.g. an existing wall/structure).
	// This avoids creating unwanted rim voxels when extruding on a flat surface.
	const bool hasLateralOffset = _offsetU != 0 || _offsetV != 0;
	if (!carving && !hasLateralOffset) {
		static constexpr int NumPerpOffsets = 4; // 2 perpendicular axes * 2 directions each

		glm::ivec3 perpOffsets[NumPerpOffsets] = {};
		perpOffsets[0][perp1] = 1;
		perpOffsets[1][perp1] = -1;
		perpOffsets[2][perp2] = 1;
		perpOffsets[3][perp2] = -1;

		for (const glm::ivec3 &selPos : selectedPositions) {
			for (int pi = 0; pi < NumPerpOffsets; ++pi) {
				const glm::ivec3 neighborPos = selPos + perpOffsets[pi];
				if (!volRegion.containsPoint(neighborPos)) {
					continue;
				}
				const voxel::Voxel &nv = vol->voxel(neighborPos);
				if (voxel::isAir(nv.getMaterial())) {
					continue; // air neighbor - no edge to seal
				}
				// Skip if the neighbor is also a selected voxel (no open edge between them).
				bool neighborSelected = false;
				for (const glm::ivec3 &other : selectedPositions) {
					if (other == neighborPos) {
						neighborSelected = true;
						break;
					}
				}
				if (neighborSelected) {
					continue;
				}
				// Only fill when the neighbor has pre-existing geometry extending in the
				// extrusion direction. On a flat surface the neighbor has nothing above
				// its base level, so no wall is needed. Check the original (pre-extrusion)
				// voxel state via history to avoid false positives from extrusion overlap.
				const glm::ivec3 aboveNeighbor = neighborPos + dir;
				if (!volRegion.containsPoint(aboveNeighbor)) {
					continue;
				}
				voxel::Voxel aboveOriginal = vol->voxel(aboveNeighbor);
				for (const HistoryEntry &entry : _history) {
					if (entry.pos == aboveNeighbor) {
						aboveOriginal = entry.original;
						break;
					}
				}
				if (voxel::isAir(aboveOriginal.getMaterial())) {
					continue;
				}
				for (int step = 1; step <= steps; ++step) {
					const glm::ivec3 shift = lateralShift(step);
					const glm::ivec3 sidePos = selPos + perpOffsets[pi] + dir * step + shift;
					if (!volRegion.containsPoint(sidePos)) {
						break;
					}
					writeVoxel(sidePos, ctx.cursorVoxel);
				}
			}
		}
	}

	// For outward extrusion: remove any voxel that is now fully interior
	// (all 6 axis-aligned neighbors solid and within bounds) - invisible voxels waste sparse storage.
	// Running after fill-sides gives accurate neighbor state before pruning.
	// Important: collect ALL positions to prune first (read-only pass), then remove them.
	// Pruning one-by-one would mutate the volume mid-iteration, making adjacent voxels
	// appear non-interior and creating a checkerboard artifact.
	if (!carving) {
		auto isInterior = [&](const glm::ivec3 &pos) {
			for (int ni = 0; ni < lengthof(voxel::arrayPathfinderFaces); ++ni) {
				const glm::ivec3 nb = pos + voxel::arrayPathfinderFaces[ni];
				if (!volRegion.containsPoint(nb) || voxel::isAir(vol->voxel(nb).getMaterial())) {
					return false;
				}
			}
			return true;
		};

		// Collect all interior positions before modifying the volume.
		core::DynamicArray<glm::ivec3> toPrune;
		toPrune.reserve(_history.size());

		// Check newly placed voxels (already in history).
		for (const HistoryEntry &entry : _history) {
			const glm::ivec3 &pos = entry.pos;
			if (!voxel::isAir(vol->voxel(pos).getMaterial()) && isInterior(pos)) {
				toPrune.push_back(pos);
			}
		}

		// Check selected voxels that outward extrusion has now fully surrounded.
		// Save them to history first so a depth change can restore them.
		for (int z = extLo.z; z <= extHi.z; ++z) {
			for (int y = extLo.y; y <= extHi.y; ++y) {
				for (int x = extLo.x; x <= extHi.x; ++x) {
					const voxel::Voxel &sv = vol->voxel(x, y, z);
					if (voxel::isAir(sv.getMaterial()) || !(sv.getFlags() & voxel::FlagOutline)) {
						continue;
					}
					const glm::ivec3 pos(x, y, z);
					if (!isInterior(pos)) {
						continue;
					}
					bool alreadySaved = false;
					for (const HistoryEntry &entry : _history) {
						if (entry.pos == pos) {
							alreadySaved = true;
							break;
						}
					}
					if (!alreadySaved) {
						_history.push_back({pos, sv});
					}
					toPrune.push_back(pos);
				}
			}
		}

		// Now prune all collected positions at once.
		for (const glm::ivec3 &pos : toPrune) {
			vol->setVoxel(pos, air);
		}
		wrapper.addToDirtyRegion(toPrune);
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
