/**
 * @file
 */

#include "ExtrudeBrush.h"
#include "math/Axis.h"
#include "voxedit-util/modifier/ModifierType.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"

namespace voxedit {

void ExtrudeBrush::reset() {
	Super::reset();
	_depth = 0;
	_face = voxel::FaceNames::Max;
	_active = false;
	_history.clear();
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

bool ExtrudeBrush::active() const {
	return _active;
}

voxel::Region ExtrudeBrush::calcRegion(const BrushContext &ctx) const {
	return ctx.targetVolumeRegion;
}

void ExtrudeBrush::setDepth(int depth) {
	_depth = depth;
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

	// Positive depth → extrude outward (along face normal, place voxels)
	// Negative depth → carve inward   (opposite to face normal, erase voxels)
	const bool carving = _depth < 0;
	const int steps = glm::abs(_depth);
	const int dirSign = carving ? -faceSign : faceSign;

	glm::ivec3 dir(0);
	dir[axisIdx] = dirSign;

	// Save original voxel at pos (only once per session) then write the new voxel.
	auto writeVoxel = [&](const glm::ivec3 &pos, const voxel::Voxel &newVoxel) {
		if (!volRegion.containsPoint(pos)) {
			return;
		}
		// Linear search — history is small enough that this is fine.
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

	// Compute the bounding box of selected (FlagOutline) voxels so the loops below
	// only scan the relevant sub-region instead of the full volume (which can be 256³+).
	glm::ivec3 selLo(volRegion.getUpperCorner());
	glm::ivec3 selHi(volRegion.getLowerCorner());
	{
		const glm::ivec3 &vlo = volRegion.getLowerCorner();
		const glm::ivec3 &vhi = volRegion.getUpperCorner();
		for (int z = vlo.z; z <= vhi.z; ++z) {
			for (int y = vlo.y; y <= vhi.y; ++y) {
				for (int x = vlo.x; x <= vhi.x; ++x) {
					const voxel::Voxel v = vol->voxel(x, y, z);
					if (!voxel::isAir(v.getMaterial()) && (v.getFlags() & voxel::FlagOutline)) {
						selLo = glm::min(selLo, glm::ivec3(x, y, z));
						selHi = glm::max(selHi, glm::ivec3(x, y, z));
					}
				}
			}
		}
	}
	if (selLo.x > selHi.x) {
		return; // no selected voxels
	}
	const glm::ivec3 &lo = selLo;
	const glm::ivec3 &hi = selHi;

	// Carve or extrude each selected voxel along dir.
	for (int z = lo.z; z <= hi.z; ++z) {
		for (int y = lo.y; y <= hi.y; ++y) {
			for (int x = lo.x; x <= hi.x; ++x) {
				const voxel::Voxel v = vol->voxel(x, y, z);
				if (voxel::isAir(v.getMaterial()) || !(v.getFlags() & voxel::FlagOutline)) {
					continue;
				}
					// Carving starts at the selected voxel itself (step=0) so the surface layer
					// becomes air. Extrusion starts at step=1 to place new voxels outward.
				const int stepFirst = carving ? 0 : 1;
				const int stepLast = carving ? steps - 1 : steps;
				for (int step = stepFirst; step <= stepLast; ++step) {
					const glm::ivec3 newPos(x + dir.x * step, y + dir.y * step, z + dir.z * step);
					if (!volRegion.containsPoint(newPos)) {
						break;
					}
					writeVoxel(newPos, carving ? air : ctx.cursorVoxel);
				}
			}
		}
	}

	// Fill perpendicular side walls — only meaningful when adding material outward.
	if (_fillSides && !carving) {
		static constexpr int NumAxes = 3;
		static constexpr int NumPerpOffsets = 4; // 2 perpendicular axes × 2 directions each
		const int perp1 = (axisIdx + 1) % NumAxes;
		const int perp2 = (axisIdx + 2) % NumAxes;

		glm::ivec3 perpOffsets[NumPerpOffsets] = {};
		perpOffsets[0][perp1] = 1;
		perpOffsets[1][perp1] = -1;
		perpOffsets[2][perp2] = 1;
		perpOffsets[3][perp2] = -1;

		for (int z = lo.z; z <= hi.z; ++z) {
			for (int y = lo.y; y <= hi.y; ++y) {
				for (int x = lo.x; x <= hi.x; ++x) {
					const voxel::Voxel &sv = vol->voxel(x, y, z);
					if (voxel::isAir(sv.getMaterial()) || !(sv.getFlags() & voxel::FlagOutline)) {
						continue;
					}
					for (int pi = 0; pi < NumPerpOffsets; ++pi) {
						const glm::ivec3 neighborPos(x + perpOffsets[pi].x,
													 y + perpOffsets[pi].y,
													 z + perpOffsets[pi].z);
						if (volRegion.containsPoint(neighborPos)) {
							const voxel::Voxel &nv = vol->voxel(neighborPos);
							if (!voxel::isAir(nv.getMaterial()) && (nv.getFlags() & voxel::FlagOutline)) {
								continue; // neighbor is also selected — no open edge
							}
						}
						for (int step = 1; step <= steps; ++step) {
							const glm::ivec3 sidePos(x + perpOffsets[pi].x + dir.x * step,
													 y + perpOffsets[pi].y + dir.y * step,
													 z + perpOffsets[pi].z + dir.z * step);
							if (!volRegion.containsPoint(sidePos)) {
								break;
							}
							writeVoxel(sidePos, ctx.cursorVoxel);
						}
					}
				}
			}
		}
	}

	// For outward extrusion: remove any voxel that is now fully interior
	// (all 6 axis-aligned neighbors solid and within bounds) — invisible voxels waste sparse storage.
	// Running after fill-sides gives accurate neighbor state before pruning.
	// Pruned selected (FlagOutline) voxels are saved to history so depth changes restore them.
	if (!carving) {
		static constexpr int NumNeighborAxes = 6;
		static const glm::ivec3 neighborOffsets[NumNeighborAxes] = {
			{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};

		auto isInterior = [&](const glm::ivec3 &pos) {
			for (int ni = 0; ni < NumNeighborAxes; ++ni) {
				const glm::ivec3 nb = pos + neighborOffsets[ni];
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

		// Prune original selected voxels that outward extrusion has now surrounded.
		// Save them to history first so a depth change can restore them.
		for (int z = lo.z; z <= hi.z; ++z) {
			for (int y = lo.y; y <= hi.y; ++y) {
				for (int x = lo.x; x <= hi.x; ++x) {
					const voxel::Voxel &sv = vol->voxel(x, y, z);
					if (voxel::isAir(sv.getMaterial()) || !(sv.getFlags() & voxel::FlagOutline)) {
						continue;
					}
					const glm::ivec3 pos(x, y, z);
					if (!isInterior(pos)) {
						continue;
					}
					bool alreadySaved = false;
					for (const HistoryEntry &e : _history) {
						if (e.pos == pos) {
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
