/**
 * @file
 */

#include "SculptBrush.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/DynamicMap.h"
#include "core/collection/DynamicSet.h"
#include "core/GLM.h"
#include "math/Axis.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/Connectivity.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/SparseVolume.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeVisitor.h"
#include <climits>

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
}

bool SculptBrush::onDeactivated() {
	return _hasSnapshot;
}

void SculptBrush::reset() {
	Super::reset();
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
	_sculptMode = SculptMode::Erode;
	_flattenFace = voxel::FaceNames::Max;
}

bool SculptBrush::beginBrush(const BrushContext &ctx) {
	if (_active) {
		return false;
	}
	if (_sculptMode == SculptMode::Flatten && ctx.cursorFace != voxel::FaceNames::Max) {
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

void SculptBrush::applySmooth(ModifierVolumeWrapper &wrapper, const BrushContext &ctx) {
	using PositionSet = core::DynamicSet<glm::ivec3, 1031, glm::hash<glm::ivec3>>;

	// Build the current solid set from the snapshot
	PositionSet currentSolid;
	// Map positions to their voxel data for color preservation
	core::DynamicMap<glm::ivec3, voxel::Voxel, 1031, glm::hash<glm::ivec3>> voxelMap;

	const glm::ivec3 &snapLo = _snapshotRegion.getLowerCorner();
	const glm::ivec3 &snapHi = _snapshotRegion.getUpperCorner();
	for (int z = snapLo.z; z <= snapHi.z; ++z) {
		for (int y = snapLo.y; y <= snapHi.y; ++y) {
			for (int x = snapLo.x; x <= snapHi.x; ++x) {
				if (!_snapshot.hasVoxel(x, y, z)) {
					continue;
				}
				const glm::ivec3 pos(x, y, z);
				currentSolid.insert(pos);
				voxelMap.put(pos, _snapshot.voxel(x, y, z));
			}
		}
	}

	voxel::RawVolume *vol = wrapper.volume();
	const voxel::Region &volRegion = vol->region();

	// Build anchor set: non-selected solid neighbors that act as immovable constraints.
	PositionSet anchorSolid;
	for (auto iter = currentSolid.begin(); iter != currentSolid.end(); ++iter) {
		const glm::ivec3 &pos = iter->key;
		for (const glm::ivec3 &offset : voxel::arrayPathfinderFaces) {
			const glm::ivec3 neighbor = pos + offset;
			if (currentSolid.has(neighbor)) {
				continue;
			}
			if (!volRegion.containsPoint(neighbor)) {
				continue;
			}
			const voxel::Voxel &v = vol->voxel(neighbor);
			if (voxel::isBlocked(v.getMaterial()) && !(v.getFlags() & voxel::FlagOutline)) {
				anchorSolid.insert(neighbor);
			}
		}
	}

	auto isSolid = [&](const glm::ivec3 &pos) -> bool {
		return currentSolid.has(pos) || anchorSolid.has(pos);
	};

	// Count solid 6-face neighbors
	auto countFaceNeighbors = [&](const glm::ivec3 &pos) -> int {
		int count = 0;
		for (const glm::ivec3 &offset : voxel::arrayPathfinderFaces) {
			if (isSolid(pos + offset)) {
				count++;
			}
		}
		return count;
	};

	if (_sculptMode == SculptMode::Erode) {
		// Erode: remove surface voxels based on their solid face-neighbor count.
		// Voxels with fewer solid neighbors are more exposed (protrusions/corners/edges).
		// Strength controls the threshold - which connectivity tier gets removed:
		//   strength=0 -> threshold=0 (nothing removed)
		//   strength~0.25 -> threshold=2 (isolated protrusions with 0-1 neighbors)
		//   strength~0.5 -> threshold=3 (also corners with 2 neighbors)
		//   strength~0.75 -> threshold=4 (also edges with 3 neighbors)
		//   strength=1.0 -> threshold=5 (also flat faces with 4 neighbors)
		// Each iteration peels one layer at the current threshold.
		// Anchors (non-selected solid) count as neighbors, so voxels touching
		// non-selected geometry are more connected and less likely to be removed.
		const int removeThreshold = (int)glm::mix(0.0f, 5.0f, _strength);

		core::DynamicArray<glm::ivec3> toRemove;
		for (int iter = 0; iter < _iterations; ++iter) {
			toRemove.clear();

			for (auto it = currentSolid.begin(); it != currentSolid.end(); ++it) {
				const glm::ivec3 &pos = it->key;
				const int neighborCount = countFaceNeighbors(pos);
				if (neighborCount == 6) {
					continue;
				}

				if (neighborCount < removeThreshold) {
					toRemove.push_back(pos);
				}
			}

			if (toRemove.empty()) {
				break;
			}

			for (const glm::ivec3 &pos : toRemove) {
				currentSolid.remove(pos);
				voxelMap.remove(pos);
			}
		}
	} else if (_sculptMode == SculptMode::Grow) {
		// Grow: fill air positions adjacent to the surface based on their neighbor count.
		// Air with more solid neighbors is more "surrounded" (concavity/gap).
		// Strength controls the minimum neighbor count needed to fill:
		//   strength=0 -> addThreshold=7 (nothing added, impossible)
		//   strength~0.25 -> addThreshold=5 (only deeply recessed holes)
		//   strength~0.5 -> addThreshold=4 (moderate concavities)
		//   strength=1.0 -> addThreshold=1 (any air touching a solid voxel)
		// Each iteration grows one layer at the current threshold.
		const int addThreshold = (int)glm::mix(7.0f, 1.0f, _strength);

		core::DynamicArray<glm::ivec3> toAdd;
		PositionSet airCandidates;
		for (int iter = 0; iter < _iterations; ++iter) {
			toAdd.clear();
			airCandidates.clear();

			for (auto it = currentSolid.begin(); it != currentSolid.end(); ++it) {
				const glm::ivec3 &pos = it->key;
				for (const glm::ivec3 &offset : voxel::arrayPathfinderFaces) {
					const glm::ivec3 neighbor = pos + offset;
					if (!isSolid(neighbor) && volRegion.containsPoint(neighbor)) {
						airCandidates.insert(neighbor);
					}
				}
			}

			for (auto it = airCandidates.begin(); it != airCandidates.end(); ++it) {
				const glm::ivec3 &pos = it->key;
				const int myCount = countFaceNeighbors(pos);
				if (myCount >= addThreshold) {
					toAdd.push_back(pos);
				}
			}

			if (toAdd.empty()) {
				break;
			}

			for (const glm::ivec3 &pos : toAdd) {
				currentSolid.insert(pos);
				// Pick color from nearest solid face-neighbor
				voxel::Voxel newVoxel = ctx.cursorVoxel;
				for (const glm::ivec3 &offset : voxel::arrayPathfinderFaces) {
					const glm::ivec3 neighbor = pos + offset;
					auto found = voxelMap.find(neighbor);
					if (found != voxelMap.end()) {
						newVoxel = found->second;
						break;
					}
				}
				newVoxel.setFlags(voxel::FlagOutline);
				voxelMap.put(pos, newVoxel);
			}
		}
	} else if (_sculptMode == SculptMode::Flatten && _flattenFace != voxel::FaceNames::Max) {
		// Flatten: peel layers from the outermost surface along the clicked face normal.
		// The face encodes both axis and direction. PositiveY means peel from top (remove highest Y layer).
		const int axisIdx = math::getIndexForAxis(voxel::faceToAxis(_flattenFace));
		const bool fromPositive = voxel::isPositiveFace(_flattenFace);

		core::DynamicArray<glm::ivec3> toRemove;
		for (int iter = 0; iter < _iterations; ++iter) {
			int extremeVal = fromPositive ? INT_MIN : INT_MAX;
			for (auto it = currentSolid.begin(); it != currentSolid.end(); ++it) {
				const glm::ivec3 &pos = it->key;
				if (fromPositive) {
					extremeVal = glm::max(extremeVal, pos[axisIdx]);
				} else {
					extremeVal = glm::min(extremeVal, pos[axisIdx]);
				}
			}

			toRemove.clear();
			for (auto it = currentSolid.begin(); it != currentSolid.end(); ++it) {
				const glm::ivec3 &pos = it->key;
				if (pos[axisIdx] == extremeVal) {
					toRemove.push_back(pos);
				}
			}

			if (toRemove.empty()) {
				break;
			}

			for (const glm::ivec3 &pos : toRemove) {
				currentSolid.remove(pos);
				voxelMap.remove(pos);
			}
		}
	}

	// Write the result: remove voxels that were in snapshot but not in result,
	// add voxels that are in result but not in snapshot
	const voxel::Voxel air;

	// Remove snapshot voxels that were smoothed away
	for (int z = snapLo.z; z <= snapHi.z; ++z) {
		for (int y = snapLo.y; y <= snapHi.y; ++y) {
			for (int x = snapLo.x; x <= snapHi.x; ++x) {
				if (!_snapshot.hasVoxel(x, y, z)) {
					continue;
				}
				const glm::ivec3 pos(x, y, z);
				if (!currentSolid.has(pos)) {
					writeVoxel(wrapper, pos, air);
				}
			}
		}
	}

	// Write all solid voxels with FlagOutline
	for (auto it = currentSolid.begin(); it != currentSolid.end(); ++it) {
		const glm::ivec3 &pos = it->key;
		auto found = voxelMap.find(pos);
		if (found != voxelMap.end()) {
			voxel::Voxel v = found->second;
			v.setFlags(voxel::FlagOutline);
			writeVoxel(wrapper, pos, v);
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

	applySmooth(wrapper, ctx);
	markDirty();
}

} // namespace voxedit
