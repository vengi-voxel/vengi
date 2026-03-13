/**
 * @file
 */

#include "SculptBrush.h"
#include "core/collection/DynamicArray.h"
#include "core/GLM.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/BitVolume.h"
#include "voxel/Connectivity.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/SparseVolume.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeSculpt.h"

namespace voxedit {

void SculptBrush::onSceneChange() {
	Super::onSceneChange();
	_active = false;
	_hasSnapshot = false;
	_snapshot.clear();
	_history.clear();
	_snapshotRegion = voxel::Region::InvalidRegion;
	_cachedRegion = voxel::Region::InvalidRegion;
	_cachedRegionValid = false;
}

void SculptBrush::onActivated() {
	reset();
	// Suppress undo registration during preview - only the final commit should create an undo entry
	_sceneModifiedFlags = SceneModifiedFlags::NoUndo;
}

bool SculptBrush::onDeactivated() {
	// Restore undo registration so the final execute in setBrushType() records the undo entry
	_sceneModifiedFlags = SceneModifiedFlags::All;
	return _hasSnapshot;
}

void SculptBrush::reset() {
	Super::reset();
	_sceneModifiedFlags = SceneModifiedFlags::All;
	_active = false;
	_hasSnapshot = false;
	_snapshot.clear();
	_history.clear();
	_snapshotRegion = voxel::Region::InvalidRegion;
	_cachedRegion = voxel::Region::InvalidRegion;
	_cachedRegionValid = false;
	_capturedVolumeLower = glm::ivec3(0);
	_strength = 0.5f;
	_iterations = 1;
	_heightThreshold = 1;
	_preserveTopHeight = false;
	_trimPerStep = 1;
	_sculptMode = SculptMode::Erode;
	_flattenFace = voxel::FaceNames::Max;
}

bool SculptBrush::beginBrush(const BrushContext &ctx) {
	if (_active) {
		return false;
	}
	const bool needsFace = _sculptMode == SculptMode::Flatten || _sculptMode == SculptMode::SmoothAdditive || _sculptMode == SculptMode::SmoothErode;
	if (needsFace && ctx.cursorFace != voxel::FaceNames::Max) {
		_flattenFace = ctx.cursorFace;
	}
	_active = true;
	return true;
}

void SculptBrush::endBrush(BrushContext &) {
	_active = false;
}

bool SculptBrush::active() const {
	return _active || _hasSnapshot;
}

void SculptBrush::preExecute(const BrushContext &ctx, const voxel::RawVolume *volume) {
	if (!_hasSnapshot && volume != nullptr) {
		captureSnapshot(volume, ctx.targetVolumeRegion);
	} else if (_hasSnapshot) {
		const glm::ivec3 delta = ctx.targetVolumeRegion.getLowerCorner() - _capturedVolumeLower;
		if (delta != glm::ivec3(0)) {
			adjustSnapshotForRegionShift(delta);
		}
	}
	if (_hasSnapshot) {
		// Expand by iterations since smoothing can grow surface by 1 voxel per iteration
		_cachedRegion = _snapshotRegion;
		_cachedRegion.grow(_iterations);
		_cachedRegionValid = true;
	}
}

voxel::Region SculptBrush::calcRegion(const BrushContext &ctx) const {
	if (_cachedRegionValid) {
		return _cachedRegion;
	}
	return ctx.targetVolumeRegion;
}

void SculptBrush::captureSnapshot(const voxel::RawVolume *volume, const voxel::Region &volRegion) {
	_snapshot.clear();
	glm::ivec3 selLo(volRegion.getUpperCorner());
	glm::ivec3 selHi(volRegion.getLowerCorner());

	const glm::ivec3 &regionLo = volRegion.getLowerCorner();
	const glm::ivec3 &regionHi = volRegion.getUpperCorner();
	for (int z = regionLo.z; z <= regionHi.z; ++z) {
		for (int y = regionLo.y; y <= regionHi.y; ++y) {
			for (int x = regionLo.x; x <= regionHi.x; ++x) {
				const voxel::Voxel &currentVoxel = volume->voxel(x, y, z);
				if (voxel::isAir(currentVoxel.getMaterial())) {
					continue;
				}
				if (!(currentVoxel.getFlags() & voxel::FlagOutline)) {
					continue;
				}
				const glm::ivec3 pos(x, y, z);
				_snapshot.setVoxel(pos, currentVoxel);
				selLo = glm::min(selLo, pos);
				selHi = glm::max(selHi, pos);
			}
		}
	}

	if (_snapshot.empty()) {
		_hasSnapshot = false;
		return;
	}

	_snapshotRegion = voxel::Region(selLo, selHi);
	_capturedVolumeLower = volRegion.getLowerCorner();
	_hasSnapshot = true;
}

void SculptBrush::adjustSnapshotForRegionShift(const glm::ivec3 &delta) {
	struct ShiftEntry {
		glm::ivec3 pos;
		voxel::Voxel voxel;
	};
	core::DynamicArray<ShiftEntry> entries;
	entries.reserve(_snapshot.size());
	const glm::ivec3 &lo = _snapshotRegion.getLowerCorner();
	const glm::ivec3 &hi = _snapshotRegion.getUpperCorner();
	for (int z = lo.z; z <= hi.z; ++z) {
		for (int y = lo.y; y <= hi.y; ++y) {
			for (int x = lo.x; x <= hi.x; ++x) {
				if (!_snapshot.hasVoxel(x, y, z)) {
					continue;
				}
				entries.push_back({glm::ivec3(x + delta.x, y + delta.y, z + delta.z), _snapshot.voxel(x, y, z)});
			}
		}
	}
	_snapshot.clear();
	for (const ShiftEntry &e : entries) {
		_snapshot.setVoxel(e.pos, e.voxel);
	}

	_snapshotRegion.shift(delta.x, delta.y, delta.z);
	_capturedVolumeLower += delta;

	struct EntryCollector {
		core::DynamicArray<ShiftEntry> *entries;
		glm::ivec3 delta;
		bool setVoxel(int x, int y, int z, const voxel::Voxel &voxel) {
			entries->push_back({glm::ivec3(x + delta.x, y + delta.y, z + delta.z), voxel});
			return true;
		}
	};
	core::DynamicArray<ShiftEntry> historyEntries;
	historyEntries.reserve(_history.size());
	EntryCollector collector{&historyEntries, delta};
	_history.copyTo(collector);
	_history.clear();
	for (const ShiftEntry &e : historyEntries) {
		_history.setVoxel(e.pos, e.voxel);
	}
}

void SculptBrush::saveToHistory(voxel::RawVolume *vol, const glm::ivec3 &pos) {
	if (_history.hasVoxel(pos)) {
		return;
	}
	_history.setVoxel(pos, vol->voxel(pos));
}

void SculptBrush::writeVoxel(ModifierVolumeWrapper &wrapper, const glm::ivec3 &pos, const voxel::Voxel &newVoxel) {
	voxel::RawVolume *volume = wrapper.volume();
	if (!volume->region().containsPoint(pos)) {
		return;
	}
	saveToHistory(volume, pos);
	if (volume->setVoxel(pos, newVoxel)) {
		wrapper.addToDirtyRegion(pos);
	}
}

void SculptBrush::applySculpt(ModifierVolumeWrapper &wrapper, const BrushContext &ctx) {
	voxel::BitVolume currentSolid(_snapshotRegion);
	voxel::SparseVolume voxelMap;

	const glm::ivec3 &snapLo = _snapshotRegion.getLowerCorner();
	const glm::ivec3 &snapHi = _snapshotRegion.getUpperCorner();
	for (int z = snapLo.z; z <= snapHi.z; ++z) {
		for (int y = snapLo.y; y <= snapHi.y; ++y) {
			for (int x = snapLo.x; x <= snapHi.x; ++x) {
				if (!_snapshot.hasVoxel(x, y, z)) {
					continue;
				}
				currentSolid.setVoxel(x, y, z, true);
				voxelMap.setVoxel(x, y, z, _snapshot.voxel(x, y, z));
			}
		}
	}

	voxel::RawVolume *vol = wrapper.volume();
	const voxel::Region &volRegion = vol->region();

	// Build anchor set: non-selected solid neighbors that act as immovable constraints.
	voxel::Region anchorRegion = _snapshotRegion;
	anchorRegion.grow(1);
	anchorRegion.cropTo(volRegion);
	voxel::BitVolume anchorSolid(anchorRegion);
	for (int z = snapLo.z; z <= snapHi.z; ++z) {
		for (int y = snapLo.y; y <= snapHi.y; ++y) {
			for (int x = snapLo.x; x <= snapHi.x; ++x) {
				if (!currentSolid.hasValue(x, y, z)) {
					continue;
				}
				for (const glm::ivec3 &offset : voxel::arrayPathfinderFaces) {
					const glm::ivec3 neighbor = glm::ivec3(x, y, z) + offset;
					if (currentSolid.hasValue(neighbor.x, neighbor.y, neighbor.z)) {
						continue;
					}
					if (!volRegion.containsPoint(neighbor)) {
						continue;
					}
					const voxel::Voxel &v = vol->voxel(neighbor);
					if (voxel::isBlocked(v.getMaterial()) && !(v.getFlags() & voxel::FlagOutline)) {
						anchorSolid.setVoxel(neighbor, true);
					}
				}
			}
		}
	}

	if (_sculptMode == SculptMode::Erode) {
		voxelutil::sculptErode(currentSolid, voxelMap, anchorSolid, _strength, _iterations);
	} else if (_sculptMode == SculptMode::Grow) {
		voxel::Voxel fillVoxel = ctx.cursorVoxel;
		fillVoxel.setFlags(voxel::FlagOutline);
		voxelutil::sculptGrow(currentSolid, voxelMap, anchorSolid, _strength, _iterations, fillVoxel);
	} else if (_sculptMode == SculptMode::Flatten && _flattenFace != voxel::FaceNames::Max) {
		voxelutil::sculptFlatten(currentSolid, voxelMap, _flattenFace, _iterations);
	} else if (_sculptMode == SculptMode::SmoothAdditive && _flattenFace != voxel::FaceNames::Max) {
		voxel::Voxel fillVoxel = ctx.cursorVoxel;
		fillVoxel.setFlags(voxel::FlagOutline);
		voxelutil::sculptSmoothAdditive(currentSolid, voxelMap, anchorSolid, _flattenFace, _heightThreshold,
										_iterations, fillVoxel);
	} else if (_sculptMode == SculptMode::SmoothErode && _flattenFace != voxel::FaceNames::Max) {
		voxelutil::sculptSmoothErode(currentSolid, voxelMap, anchorSolid, _flattenFace, _iterations, _preserveTopHeight,
								_trimPerStep);
	}

	// Write the result: remove voxels that were in snapshot but not in result,
	// add voxels that are in result but not in snapshot
	const voxel::Voxel air;

	// Remove snapshot voxels that were sculpted away
	for (int z = snapLo.z; z <= snapHi.z; ++z) {
		for (int y = snapLo.y; y <= snapHi.y; ++y) {
			for (int x = snapLo.x; x <= snapHi.x; ++x) {
				if (!_snapshot.hasVoxel(x, y, z)) {
					continue;
				}
				const glm::ivec3 pos(x, y, z);
				if (!currentSolid.hasValue(x, y, z)) {
					writeVoxel(wrapper, pos, air);
				}
			}
		}
	}

	// Write all solid voxels with FlagOutline
	for (int z = snapLo.z; z <= snapHi.z; ++z) {
		for (int y = snapLo.y; y <= snapHi.y; ++y) {
			for (int x = snapLo.x; x <= snapHi.x; ++x) {
				if (!currentSolid.hasValue(x, y, z)) {
					continue;
				}
				const glm::ivec3 pos(x, y, z);
				if (voxelMap.hasVoxel(x, y, z)) {
					voxel::Voxel v = voxelMap.voxel(x, y, z);
					v.setFlags(voxel::FlagOutline);
					writeVoxel(wrapper, pos, v);
				}
			}
		}
	}
}

void SculptBrush::generate(scenegraph::SceneGraph &, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
						   const voxel::Region &) {
	if (!_hasSnapshot) {
		return;
	}

	voxel::RawVolume *vol = wrapper.volume();

	// Restore previously modified state before re-applying
	struct HistoryRestorer {
		voxel::RawVolume *vol;
		ModifierVolumeWrapper *wrapper;
		bool setVoxel(int x, int y, int z, const voxel::Voxel &voxel) {
			if (vol->setVoxel(x, y, z, voxel)) {
				wrapper->addToDirtyRegion(glm::ivec3(x, y, z));
			}
			return true;
		}
	};
	HistoryRestorer restorer{vol, &wrapper};
	_history.copyTo(restorer);
	_history.clear();

	applySculpt(wrapper, ctx);
	markDirty();
}

} // namespace voxedit
