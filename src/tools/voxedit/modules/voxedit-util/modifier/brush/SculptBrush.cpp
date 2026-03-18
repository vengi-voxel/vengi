/**
 * @file
 */

#include "SculptBrush.h"
#include "core/Trace.h"
#include "core/collection/DynamicArray.h"
#include "core/GLM.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/BitVolume.h"
#include "voxel/Connectivity.h"
#include "voxel/DynamicVoxelArray.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/Region.h"
#include "voxel/SparseVolume.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeSculpt.h"
#include "voxelutil/VolumeVisitor.h"

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

bool SculptBrush::hasPendingChanges() const {
	return _hasSnapshot;
}

voxel::Region SculptBrush::revertChanges(voxel::RawVolume *volume) {
	voxel::RawVolumeWrapper wrapper(volume);
	_history.copyTo(wrapper);
	_history.clear();
	return wrapper.dirtyRegion();
}

bool SculptBrush::onDeactivated() {
	// Restore undo registration so the final execute in setBrushType() records the undo entry
	_sceneModifiedFlags = SceneModifiedFlags::All;
	return hasPendingChanges();
}

void SculptBrush::reset() {
	Super::reset();
	_sceneModifiedFlags = SceneModifiedFlags::All;
	_active = false;
	_hasSnapshot = false;
	_paramsDirty = true;
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
	_kernelSize = 4;
	_sigma = 4.0f;
	_sculptMode = SculptMode::Erode;
	_flattenFace = voxel::FaceNames::Max;
}

bool SculptBrush::beginBrush(const BrushContext &ctx) {
	if (_active) {
		return false;
	}
	const bool needsFace = modeNeedsFace(_sculptMode);
	if (needsFace && ctx.cursorFace != voxel::FaceNames::Max) {
		_flattenFace = ctx.cursorFace;
		if (_sculptMode == SculptMode::SquashToPlane) {
			const int axisIdx = math::getIndexForAxis(voxel::faceToAxis(ctx.cursorFace));
			_squashPlaneCoord = ctx.cursorPosition[axisIdx];
		}
		_paramsDirty = true;
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
		_cachedRegion = _snapshotRegion;
		if (_sculptMode == SculptMode::Reskin) {
			// Expand along face normal for z offset + skin layers growing outward
			const int expand = glm::abs(_reskinConfig.zOffset) + _reskinConfig.skinDepth;
			_cachedRegion.grow(expand);
		} else {
			// Expand by iterations since smoothing can grow surface by 1 voxel per iteration
			_cachedRegion.grow(_iterations);
		}
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
	core_trace_scoped(SculptBrushCaptureSnapshot);
	_snapshot.clear();
	glm::ivec3 selLo(volRegion.getUpperCorner());
	glm::ivec3 selHi(volRegion.getLowerCorner());

	voxelutil::visitVolume(*volume, volRegion, [&] (int x, int y, int z, const voxel::Voxel &voxel) {
		const glm::ivec3 pos(x, y, z);
		_snapshot.setVoxel(pos, voxel);
		selLo = glm::min(selLo, pos);
		selHi = glm::max(selHi, pos);
	}, voxelutil::VisitSolidOutline());

	if (_snapshot.empty()) {
		_hasSnapshot = false;
		return;
	}

	_snapshotRegion = voxel::Region(selLo, selHi);
	_capturedVolumeLower = volRegion.getLowerCorner();
	_hasSnapshot = true;
}

void SculptBrush::adjustSnapshotForRegionShift(const glm::ivec3 &delta) {
	core_trace_scoped(SculptBrushAdjustSnapshotForRegionShift);
	struct ShiftEntry {
		glm::ivec3 pos;
		voxel::Voxel voxel;
	};
	core::DynamicArray<ShiftEntry> entries;
	entries.reserve(_snapshotRegion.voxels());
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
	core_trace_scoped(SculptBrushApplySculpt);

	// For Reskin mode, expand the working region along the face normal to accommodate
	// the z offset and skin layers growing outward from the surface.
	voxel::Region workRegion = _snapshotRegion;
	if (_sculptMode == SculptMode::Reskin && _flattenFace != voxel::FaceNames::Max) {
		const int axisIdx = math::getIndexForAxis(voxel::faceToAxis(_flattenFace));
		const int expand = glm::abs(_reskinConfig.zOffset) + _reskinConfig.skinDepth;
		glm::ivec3 lo = workRegion.getLowerCorner();
		glm::ivec3 hi = workRegion.getUpperCorner();
		lo[axisIdx] -= expand;
		hi[axisIdx] += expand;
		// Clamp to volume bounds
		const voxel::Region &volRegion = wrapper.volume()->region();
		lo = glm::max(lo, volRegion.getLowerCorner());
		hi = glm::min(hi, volRegion.getUpperCorner());
		workRegion = voxel::Region(lo, hi);
	}

	voxel::BitVolume currentSolid(workRegion);
	voxel::SparseVolume voxelMap;

	const glm::ivec3 &snapLo = _snapshotRegion.getLowerCorner();
	const glm::ivec3 &snapHi = _snapshotRegion.getUpperCorner();

	// Collect snapshot entries: positions + voxels for reuse in write-back phase
	voxel::DynamicVoxelArray snapshotEntries;
	snapshotEntries.reserve(_snapshot.size());

	struct SnapshotLoader {
		voxel::BitVolume *solid;
		voxel::SparseVolume *voxelMap;
		voxel::DynamicVoxelArray *entries;
		bool setVoxel(int x, int y, int z, const voxel::Voxel &v) {
			const glm::ivec3 pos(x, y, z);
			solid->setVoxel(x, y, z, true);
			voxelMap->setVoxel(pos, v);
			entries->push_back({pos, v});
			return true;
		}
	};
	SnapshotLoader loader{&currentSolid, &voxelMap, &snapshotEntries};
	_snapshot.copyTo(loader);

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

	// Save snapshot positions as a BitVolume before sculpt modifies currentSolid.
	// Used later to distinguish original vs newly grown positions (O(1) bit test).
	voxel::BitVolume snapshotSolid(currentSolid);

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
	} else if (_sculptMode == SculptMode::SmoothGaussian && _flattenFace != voxel::FaceNames::Max) {
		voxel::Voxel fillVoxel = ctx.cursorVoxel;
		fillVoxel.setFlags(voxel::FlagOutline);
		voxelutil::sculptSmoothGaussian(currentSolid, voxelMap, anchorSolid, _flattenFace, _kernelSize, _sigma,
										_iterations, fillVoxel);
	} else if (_sculptMode == SculptMode::BridgeGap) {
		voxel::Voxel fillVoxel = ctx.cursorVoxel;
		fillVoxel.setFlags(voxel::FlagOutline);
		voxelutil::sculptBridgeGap(currentSolid, voxelMap, anchorSolid, fillVoxel);
	} else if (_sculptMode == SculptMode::SquashToPlane && _flattenFace != voxel::FaceNames::Max) {
		voxelutil::sculptSquashToPlane(currentSolid, voxelMap, _flattenFace, _squashPlaneCoord);
	} else if (_sculptMode == SculptMode::Reskin && _skinVolume != nullptr && _flattenFace != voxel::FaceNames::Max) {
		voxelutil::sculptReskin(currentSolid, voxelMap, *_skinVolume, _flattenFace, _reskinConfig);
	}

	// Write results using the collected snapshot entries - no hash lookups needed.
	// Snapshot entries have the original positions and colors. After sculpt,
	// currentSolid tells us what survived (BitVolume, O(1) bit test).
	const voxel::Voxel air;

	// Pass 1: remove sculpted-away voxels + write surviving snapshot voxels.
	// Read from voxelMap (not snapshot) so color changes from reskin are applied.
	// For non-reskin modes, voxelMap entries match the snapshot for surviving positions.
	for (const voxel::VoxelPosition &entry : snapshotEntries) {
		if (!currentSolid.hasValue(entry.pos.x, entry.pos.y, entry.pos.z)) {
			writeVoxel(wrapper, entry.pos, air);
		} else if (voxelMap.hasVoxel(entry.pos)) {
			voxel::Voxel v = voxelMap.voxel(entry.pos);
			v.setFlags(voxel::FlagOutline);
			writeVoxel(wrapper, entry.pos, v);
		} else {
			voxel::Voxel v = entry.voxel;
			v.setFlags(voxel::FlagOutline);
			writeVoxel(wrapper, entry.pos, v);
		}
	}

	// Pass 2: write newly grown voxels (added by sculpt, not in original snapshot).
	// Scan the working region (may be expanded for reskin) with O(1) bit tests.
	// Only new positions need voxelMap hash lookup for color.
	const glm::ivec3 &workLo = workRegion.getLowerCorner();
	const glm::ivec3 &workHi = workRegion.getUpperCorner();
	for (int z = workLo.z; z <= workHi.z; ++z) {
		for (int y = workLo.y; y <= workHi.y; ++y) {
			for (int x = workLo.x; x <= workHi.x; ++x) {
				if (!currentSolid.hasValue(x, y, z)) {
					continue;
				}
				if (snapshotSolid.hasValue(x, y, z)) {
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
	if (!_paramsDirty) {
		return;
	}
	_paramsDirty = false;

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
