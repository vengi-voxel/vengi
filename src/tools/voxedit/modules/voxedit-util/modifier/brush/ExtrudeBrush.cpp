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
	return _active;
}

voxel::Region ExtrudeBrush::calcRegion(const BrushContext &ctx) const {
	if (!_cachedSelBBoxValid || _face == voxel::FaceNames::Max) {
		return ctx.targetVolumeRegion;
	}
	// Expand the cached selection bbox to include all voxels that may be affected
	// by the extrusion: depth steps + lateral offsets + side wall margin.
	static constexpr int SideWallMargin = 1;
	const int expand = glm::abs(_depth) + glm::abs(_offsetU) + glm::abs(_offsetV) + SideWallMargin;
	voxel::Region region = _cachedSelBBox;
	region.grow(expand);
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
	// Tip voxel: same as cursor but with FlagOutline so the outermost extrusion layer
	// stays selected for chaining extrudes (e.g. building a column then bending it).
	voxel::Voxel tipVoxel = ctx.cursorVoxel;
	tipVoxel.setFlags(ctx.cursorVoxel.getFlags() | voxel::FlagOutline);

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

	// Clear FlagOutline on original selected voxels so only the tip stays selected.
	// Save them to history so depth changes can restore the original selection state.
	if (!carving) {
		for (const glm::ivec3 &pos : selectedPositions) {
			const voxel::Voxel origVoxel = vol->voxel(pos);
			writeVoxel(pos, voxel::Voxel(origVoxel.getMaterial(), origVoxel.getColor(), origVoxel.getNormal(),
										 origVoxel.getFlags() & ~voxel::FlagOutline, origVoxel.getBoneIdx()));
		}
	}

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
			// Only the outermost layer gets FlagOutline.
			const bool isTip = (step == stepLast) && !carving;
			writeVoxel(newPos, carving ? air : (isTip ? tipVoxel : ctx.cursorVoxel));
		}
		// After carving, mark the voxel behind the deepest carved layer
		// with FlagOutline so the selection moves inward (mirrors tip selection
		// for positive extrusion). This keeps visual masking active.
		if (carving) {
			const glm::ivec3 shift = lateralShift(steps);
			const glm::ivec3 behindPos = selPos + dir * steps + shift;
			if (volRegion.containsPoint(behindPos)) {
				const voxel::Voxel behind = vol->voxel(behindPos);
				if (!voxel::isAir(behind.getMaterial())) {
					voxel::Voxel flagged = behind;
					flagged.setFlags(behind.getFlags() | voxel::FlagOutline);
					writeVoxel(behindPos, flagged);
				}
			}
		}
	}

	// Expanded bounding box covering tip voxels after lateral offset + side wall margin.
	// The fill-sides and pruning loops need to find FlagOutline voxels which may have been
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
	// Pruned selected (FlagOutline) voxels are saved to history so depth changes restore them.
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

		// Prune newly placed voxels (already in history).
		for (const HistoryEntry &entry : _history) {
			const glm::ivec3 &pos = entry.pos;
			if (!voxel::isAir(vol->voxel(pos).getMaterial()) && isInterior(pos)) {
				vol->setVoxel(pos, air);
				wrapper.addToDirtyRegion(pos);
			}
		}

		// Prune tip voxels that outward extrusion has now fully surrounded.
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
					vol->setVoxel(pos, air);
					wrapper.addToDirtyRegion(pos);
				}
			}
		}
	}
}

} // namespace voxedit
