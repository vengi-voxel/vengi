/**
 * @file
 */

#include "SculptOperation.h"
#include "core/GLM.h"
#include "core/Log.h"
#include "core/Trace.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/DynamicMap.h"
#include "math/Axis.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraphNode.h"
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
#include <cmath>

namespace voxedit {

bool SculptOperation::modeNeedsFace(SculptMode mode) {
	return mode == SculptMode::Flatten || mode == SculptMode::SmoothAdditive || mode == SculptMode::SmoothErode ||
		   mode == SculptMode::SmoothGaussian || mode == SculptMode::SquashToPlane || mode == SculptMode::ExtendPlane ||
		   mode == SculptMode::Reskin || mode == SculptMode::SmoothWall;
}

bool SculptOperation::setParameters(const Parameters &parameters) {
	if (parameters.mode >= SculptMode::Max || !std::isfinite(parameters.strength) || !std::isfinite(parameters.sigma) ||
		parameters.face > voxel::FaceNames::Max) {
		return false;
	}
	if (_parameters.mode != parameters.mode || _parameters.face != parameters.face) {
		_planeFitted = false;
	}
	_parameters = parameters;
	_parameters.strength = glm::clamp(parameters.strength, 0.0f, 1.0f);
	_parameters.iterations = glm::clamp(parameters.iterations, 1, 250);
	_parameters.radius = glm::clamp(parameters.radius, 1, 32);
	_parameters.kernelSize = glm::clamp(parameters.kernelSize, 1, 7);
	_parameters.sigma = glm::clamp(parameters.sigma, 0.1f, 8.0f);
	_paramsDirty = true;
	_active = true;
	return true;
}

void SculptOperation::prepare(const voxel::RawVolume *volume, const voxel::Region &region) {
	if (!_hasSnapshot && volume) {
		captureSnapshot(volume, region);
	}
}

void SculptOperation::captureSnapshot(const voxel::RawVolume *volume, const voxel::Region &volRegion) {
	core_trace_scoped(SculptBrushCaptureSnapshot);
	_snapshotEntries.clear();

	// Use regionForFlag to get tight bounding box of selected voxels first.
	// This avoids scanning the entire volume (e.g. 256^3 = 16M voxels) when the
	// selection is a small fraction of the volume (e.g. 100x100x5 = 50K voxels).
	const voxel::Region selectionBounds = volume->regionForFlag(voxel::FlagOutline);
	if (!selectionBounds.isValid()) {
		_hasSnapshot = false;
		return;
	}

	// Scan only the tight selection bounding box
	voxel::Region scanRegion = selectionBounds;
	scanRegion.cropTo(volRegion);

	// Pre-allocate to avoid repeated reallocations during push_back.
	// The actual count will be <= voxels in the scan region.
	_snapshotEntries.reserve(scanRegion.voxels());

	glm::ivec3 selLo(scanRegion.getUpperCorner());
	glm::ivec3 selHi(scanRegion.getLowerCorner());

	voxelutil::visitVolume(
		*volume, scanRegion,
		[&](int x, int y, int z, const voxel::Voxel &voxel) {
			const glm::ivec3 pos(x, y, z);
			_snapshotEntries.push_back({pos, voxel});
			selLo = glm::min(selLo, pos);
			selHi = glm::max(selHi, pos);
		},
		voxelutil::VisitSolidOutline());

	if (_snapshotEntries.empty()) {
		_hasSnapshot = false;
		return;
	}

	_snapshotRegion = voxel::Region(selLo, selHi);
	_capturedVolumeLower = volRegion.getLowerCorner();
	_hasSnapshot = true;
	_cachedBitVolumeValid = false;
	_cachedVoxelMapValid = false;
}

void SculptOperation::adjustSnapshotForRegionShift(const glm::ivec3 &delta) {
	core_trace_scoped(SculptBrushAdjustSnapshotForRegionShift);

	// Shift all snapshot entries in-place
	for (voxel::VoxelPosition &entry : _snapshotEntries) {
		entry.pos += delta;
	}

	_snapshotRegion.shift(delta.x, delta.y, delta.z);
	_capturedVolumeLower += delta;

	// Shift history entries in-place (flat array, no hash rebuild needed)
	for (voxel::VoxelPosition &entry : _historyEntries) {
		entry.pos += delta;
	}
	if (_historyRegion.isValid()) {
		_historyRegion.shift(delta.x, delta.y, delta.z);
	}
	_cachedBitVolumeValid = false;
	_cachedVoxelMapValid = false;
}

void SculptOperation::saveToHistory(voxel::RawVolume *vol, const glm::ivec3 &pos) {
	// O(1) bit test instead of O(N/buckets) hash chain traversal
	if (_historyRegion.containsPoint(pos) && _historyBits.hasValue(pos.x, pos.y, pos.z)) {
		return;
	}
	if (_historyRegion.containsPoint(pos)) {
		_historyBits.setVoxel(pos.x, pos.y, pos.z, true);
	}
	_historyEntries.push_back({pos, vol->voxel(pos)});
}

void SculptOperation::writeVoxel(ModifierVolumeWrapper &wrapper, const glm::ivec3 &pos, const voxel::Voxel &newVoxel) {
	voxel::RawVolume *volume = wrapper.volume();
	if (!volume->region().containsPoint(pos)) {
		return;
	}
	saveToHistory(volume, pos);
	if (volume->setVoxel(pos, newVoxel)) {
		wrapper.addToDirtyRegion(pos);
	}
}

void SculptOperation::rebuildCachedBitVolume() {
	if (_cachedBitVolumeValid) {
		return;
	}
	core_trace_scoped(SculptBrushRebuildBitVolume);
	_cachedBitVolume = voxel::BitVolume(_snapshotRegion);
	for (const voxel::VoxelPosition &entry : _snapshotEntries) {
		_cachedBitVolume.setVoxel(entry.pos.x, entry.pos.y, entry.pos.z, true);
	}
	_cachedBitVolumeValid = true;
	_cachedVoxelMapValid = false;
}

void SculptOperation::rebuildCachedVoxelMap() {
	if (_cachedVoxelMapValid) {
		return;
	}
	core_trace_scoped(SculptBrushRebuildVoxelMap);
	_cachedVoxelMap.clear();
	for (const voxel::VoxelPosition &entry : _snapshotEntries) {
		_cachedVoxelMap.setVoxel(entry.pos, entry.voxel);
	}
	_cachedVoxelMapValid = true;
}

void SculptOperation::applySculpt(ModifierVolumeWrapper &wrapper) {
	core_trace_scoped(SculptBrushApplySculpt);

	// Ensure the cached BitVolume is up to date (built from flat _snapshotEntries).
	rebuildCachedBitVolume();

	// ---- Reskin fast path ----
	// sculptReskin never reads from voxelMap, only writes. So we skip the expensive O(N)
	// SparseVolume construction and full write-back. Instead: copy cached BitVolume (cheap
	// bit memcpy), pass empty voxelMap, and write back only the entries sculptReskin modified.
	// ---- Reskin: early return when no skin loaded ----
	if (_parameters.mode == SculptMode::Reskin && _parameters.skin.get() == nullptr) {
		return;
	}

	if (_parameters.mode == SculptMode::Reskin && _parameters.skin.get() != nullptr &&
		_parameters.face != voxel::FaceNames::Max) {
		// Expand the working BitVolume to accommodate skin layers that protrude
		// beyond the selection. Without this, out-of-region setVoxel calls are
		// silently dropped and protruding skin voxels never appear.
		const int expand = glm::abs(_parameters.reskin.zOffset) + _parameters.reskin.skinDepth;
		voxel::Region expandedRegion = _snapshotRegion;
		expandedRegion.grow(expand);
		voxel::BitVolume workSolid(expandedRegion);
		// Copy snapshot bits into the expanded volume
		for (const voxel::VoxelPosition &entry : _snapshotEntries) {
			workSolid.setVoxel(entry.pos.x, entry.pos.y, entry.pos.z, true);
		}
		voxel::SparseVolume voxelMap;

		const palette::Palette *skinPal = skinPalette();
		palette::Palette &nodePal = wrapper.node().palette();
		voxelutil::sculptReskin(workSolid, voxelMap, *_parameters.skin.get(), _parameters.face, _parameters.reskin,
								skinPal, skinPal != nullptr ? &nodePal : nullptr);

		// Write back directly to volume, bypassing per-voxel writeVoxel/saveToHistory overhead.
		// SparseVolume iteration has no duplicates, so no dedup check needed.
		// Pre-allocate history to avoid repeated reallocations for large entries.
		voxel::RawVolume *vol = wrapper.volume();
		const voxel::Region &volRegion = vol->region();
		_historyEntries.reserve(voxelMap.size());
		voxel::Region dirtyRegion;
		struct ReskinWriter {
			voxel::RawVolume *vol;
			const voxel::Region *volRegion;
			voxel::DynamicVoxelArray *historyEntries;
			voxel::Region *dirtyRegion;
			bool setVoxel(int x, int y, int z, const voxel::Voxel &voxel) {
				if (!volRegion->containsPoint(x, y, z)) {
					return true;
				}
				// Save original voxel to history (flat array, no hash)
				historyEntries->push_back({glm::ivec3(x, y, z), vol->voxel(x, y, z)});
				// Write new voxel directly to volume
				voxel::Voxel v = voxel;
				if (voxel::isBlocked(v.getMaterial())) {
					v.setFlags(voxel::FlagOutline);
				}
				vol->setVoxel(x, y, z, v);
				if (dirtyRegion->isValid()) {
					dirtyRegion->accumulate(x, y, z);
				} else {
					*dirtyRegion = voxel::Region(x, y, z, x, y, z);
				}
				return true;
			}
		};
		ReskinWriter writer{vol, &volRegion, &_historyEntries, &dirtyRegion};
		voxelMap.copyTo(writer);
		// Accumulate dirty bounding box via two corner points (no Region overload exists)
		if (dirtyRegion.isValid()) {
			wrapper.addToDirtyRegion(dirtyRegion.getLowerCorner());
			wrapper.addToDirtyRegion(dirtyRegion.getUpperCorner());
		}
		return;
	}

	// ---- SmoothWall fast path ----
	// Use a temporary RawVolume (flat array) instead of SparseVolume (hash map).
	// Population is O(N) array-index writes vs O(N) hash insertions - orders of magnitude faster.
	if (_parameters.mode == SculptMode::SmoothWall && _parameters.face != voxel::FaceNames::Max) {
		rebuildCachedBitVolume();
		voxel::BitVolume currentSolid(_cachedBitVolume);

		voxel::RawVolume *vol = wrapper.volume();
		const voxel::Region &volRegion = vol->region();

		// Build anchors from snapshot entries (O(N) where N = selected voxels)
		voxel::Region anchorRegion = _snapshotRegion;
		anchorRegion.grow(1);
		anchorRegion.cropTo(volRegion);
		voxel::BitVolume anchorSolid(anchorRegion);
		for (const voxel::VoxelPosition &entry : _snapshotEntries) {
			for (const glm::ivec3 &offset : voxel::arrayPathfinderFaces) {
				const glm::ivec3 neighbor = entry.pos + offset;
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
		// RawVolume for color data inside the sculpt algorithm: fillSmoothWallVoxel
		// does 6 neighbor lookups per voxel, O(1) array access vs O(1) amortized hash.
		// The dense allocation is large but the random-access speed is critical for
		// step 3 which can do 100M+ lookups.
		voxel::RawVolume colorVolume(_snapshotRegion);
		for (const voxel::VoxelPosition &entry : _snapshotEntries) {
			colorVolume.setVoxel(entry.pos, entry.voxel);
		}
		voxel::Voxel fillVoxel = _parameters.voxel;
		fillVoxel.setFlags(voxel::FlagOutline);
		static constexpr int smoothWallIterations = 1;
		core::DynamicArray<glm::ivec3> addedPositions;
		voxelutil::sculptSmoothWall(currentSolid, colorVolume, anchorSolid, _parameters.face, smoothWallIterations,
									fillVoxel, _parameters.smoothWallClearDepth, _parameters.smoothWallInterp,
									_parameters.smoothWallFillHoles, addedPositions);
		const voxel::Voxel air;
		voxel::Region dirtyRegion;
		_historyEntries.reserve(_snapshotEntries.size() + addedPositions.size());

		// Removed entries (was in snapshot, no longer in currentSolid)
		for (const voxel::VoxelPosition &entry : _snapshotEntries) {
			if (!currentSolid.hasValue(entry.pos.x, entry.pos.y, entry.pos.z)) {
				if (!volRegion.containsPoint(entry.pos)) {
					continue;
				}
				saveToHistory(vol, entry.pos);
				if (vol->setVoxel(entry.pos, air)) {
					if (dirtyRegion.isValid()) {
						dirtyRegion.accumulate(entry.pos);
					} else {
						dirtyRegion = voxel::Region(entry.pos, entry.pos);
					}
				}
			}
		}
		// Added entries: collected during sculptSmoothWall
		for (const glm::ivec3 &pos : addedPositions) {
			if (!volRegion.containsPoint(pos)) {
				continue;
			}
			if (!currentSolid.hasValue(pos.x, pos.y, pos.z)) {
				continue;
			}
			const voxel::Voxel &cv = colorVolume.voxel(pos);
			if (!voxel::isBlocked(cv.getMaterial())) {
				continue;
			}
			voxel::Voxel v = cv;
			v.setFlags(voxel::FlagOutline);
			saveToHistory(vol, pos);
			if (vol->setVoxel(pos, v)) {
				if (dirtyRegion.isValid()) {
					dirtyRegion.accumulate(pos);
				} else {
					dirtyRegion = voxel::Region(pos, pos);
				}
			}
		}
		// Single bulk dirty-region update (2 calls instead of N)
		if (dirtyRegion.isValid()) {
			wrapper.addToDirtyRegion(dirtyRegion.getLowerCorner());
			wrapper.addToDirtyRegion(dirtyRegion.getUpperCorner());
		}
		return;
	}

	// ---- Generic path for all other sculpt modes ----
	voxel::Region workRegion = _snapshotRegion;

	// Erode only writes air to removed positions - it never reads voxel data from voxelMap.
	// Skip the expensive O(N) SparseVolume construction for it.
	const bool needsVoxelMap = _parameters.mode != SculptMode::Erode;

	// Reuse cached BitVolume (copy is cheap bit memcpy, since sculpt mutates it).
	// Build cached SparseVolume once and deep-copy on each call.
	rebuildCachedBitVolume();
	voxel::BitVolume currentSolid(_cachedBitVolume);
	voxel::SparseVolume voxelMap;
	if (needsVoxelMap) {
		rebuildCachedVoxelMap();
		_cachedVoxelMap.copyTo(voxelMap);
	}

	voxel::RawVolume *vol = wrapper.volume();
	const voxel::Region &volRegion = vol->region();

	// Build anchor set only for modes that use it
	const bool needsAnchors =
		_parameters.mode == SculptMode::Erode || _parameters.mode == SculptMode::Grow ||
		_parameters.mode == SculptMode::SmoothAdditive || _parameters.mode == SculptMode::SmoothErode ||
		_parameters.mode == SculptMode::SmoothGaussian || _parameters.mode == SculptMode::BridgeGap;
	voxel::Region anchorRegion = _snapshotRegion;
	anchorRegion.grow(1);
	anchorRegion.cropTo(volRegion);
	voxel::BitVolume anchorSolid(anchorRegion);
	if (needsAnchors) {
		const glm::ivec3 &snapLo = _snapshotRegion.getLowerCorner();
		const glm::ivec3 &snapHi = _snapshotRegion.getUpperCorner();
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
	}

	// Use _cachedBitVolume as snapshot reference (never mutated, avoids a full BitVolume copy).
	if (_parameters.mode == SculptMode::Erode) {
		voxelutil::sculptErode(currentSolid, voxelMap, anchorSolid, _parameters.strength, _parameters.iterations);
	} else if (_parameters.mode == SculptMode::Grow) {
		voxel::Voxel fillVoxel = _parameters.voxel;
		fillVoxel.setFlags(voxel::FlagOutline);
		voxelutil::sculptGrow(currentSolid, voxelMap, anchorSolid, _parameters.strength, _parameters.iterations,
							  fillVoxel);
	} else if (_parameters.mode == SculptMode::Flatten && _parameters.face != voxel::FaceNames::Max) {
		voxelutil::sculptFlatten(currentSolid, voxelMap, _parameters.face, _parameters.iterations);
	} else if (_parameters.mode == SculptMode::SmoothAdditive && _parameters.face != voxel::FaceNames::Max) {
		voxel::Voxel fillVoxel = _parameters.voxel;
		fillVoxel.setFlags(voxel::FlagOutline);
		voxelutil::sculptSmoothAdditive(currentSolid, voxelMap, anchorSolid, _parameters.face,
										_parameters.heightThreshold, _parameters.iterations, fillVoxel);
	} else if (_parameters.mode == SculptMode::SmoothErode && _parameters.face != voxel::FaceNames::Max) {
		voxelutil::sculptSmoothErode(currentSolid, voxelMap, anchorSolid, _parameters.face, _parameters.iterations,
									 _parameters.preserveTopHeight, _parameters.trimPerStep);
	} else if (_parameters.mode == SculptMode::SmoothGaussian && _parameters.face != voxel::FaceNames::Max) {
		voxel::Voxel fillVoxel = _parameters.voxel;
		fillVoxel.setFlags(voxel::FlagOutline);
		voxelutil::sculptSmoothGaussian(currentSolid, voxelMap, anchorSolid, _parameters.face, _parameters.kernelSize,
										_parameters.sigma, _parameters.iterations, fillVoxel);
	} else if (_parameters.mode == SculptMode::BridgeGap) {
		voxel::Voxel fillVoxel = _parameters.voxel;
		fillVoxel.setFlags(voxel::FlagOutline);
		voxelutil::sculptBridgeGap(currentSolid, voxelMap, anchorSolid, fillVoxel);
	} else if (_parameters.mode == SculptMode::SquashToPlane && _parameters.face != voxel::FaceNames::Max) {
		voxelutil::sculptSquashToPlane(currentSolid, voxelMap, _parameters.face, _parameters.plane);
	}

	// Write-back: only write entries that actually CHANGED.
	// - Removed entries (was solid, now not): write air
	// - Modified entries (in voxelMap with different value): write new value
	// - Unchanged entries: SKIP (volume already has correct values after history restore)
	// This reduces 5.6M writes to ~100K for erode, saving massive history hash overhead.
	const voxel::Voxel air;

	// Pass 1: handle snapshot entries (removed or modified by sculpt)
	for (const voxel::VoxelPosition &entry : _snapshotEntries) {
		if (!currentSolid.hasValue(entry.pos.x, entry.pos.y, entry.pos.z)) {
			// Removed by sculpt - write air
			writeVoxel(wrapper, entry.pos, air);
		} else if (needsVoxelMap && voxelMap.hasVoxel(entry.pos)) {
			// Check if the voxel was actually modified by comparing with original
			const voxel::Voxel &mapVoxel = voxelMap.voxel(entry.pos);
			if (mapVoxel.getColor() != entry.voxel.getColor() || mapVoxel.getMaterial() != entry.voxel.getMaterial() ||
				mapVoxel.getNormal() != entry.voxel.getNormal()) {
				voxel::Voxel v = mapVoxel;
				v.setFlags(voxel::FlagOutline);
				writeVoxel(wrapper, entry.pos, v);
			}
		}
		// Else: unchanged, skip - volume already has the correct original value
	}

	// Pass 2: write newly grown voxels (not in original snapshot)
	const glm::ivec3 &workLo = workRegion.getLowerCorner();
	const glm::ivec3 &workHi = workRegion.getUpperCorner();
	for (int z = workLo.z; z <= workHi.z; ++z) {
		for (int y = workLo.y; y <= workHi.y; ++y) {
			for (int x = workLo.x; x <= workHi.x; ++x) {
				if (!currentSolid.hasValue(x, y, z)) {
					continue;
				}
				if (_cachedBitVolume.hasValue(x, y, z)) {
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

void SculptOperation::evaluate(ModifierVolumeWrapper &wrapper) {
	if (!_hasSnapshot) {
		return;
	}
	if (!_paramsDirty) {
		return;
	}

	_paramsDirty = false;

	// ExtendPlane uses additive painting - no history restore between strokes
	if (_parameters.mode == SculptMode::ExtendPlane) {
		if (!_planeFitted && _parameters.face != voxel::FaceNames::Max) {
			fitPlaneFromSnapshot();
			if (_planeFitted) {
				// Initialize history tracking for dedup in saveToHistory().
				// Must happen before any painting so the first write to each position
				// is recorded as the original value.
				voxel::RawVolume *vol = wrapper.volume();
				_historyRegion = _snapshotRegion;
				_historyRegion.grow(_parameters.radius + _parameters.removeAboveDepth + 2);
				_historyRegion.cropTo(vol->region());
				_historyBits = voxel::BitVolume(_historyRegion);
			}
		}
		if (_planeFitted && _active) {
			paintExtendPlane(wrapper);
			_lastPaintPos = _parameters.cursor;
		} else if (_planeFitted && !_active && !_historyEntries.empty()) {
			// Commit path: _active is false, changes are already in the volume.
			// Mark all modified positions as dirty so the undo system captures them.
			for (const voxel::VoxelPosition &entry : _historyEntries) {
				wrapper.addToDirtyRegion(entry.pos);
			}
		}
		return;
	}

	voxel::RawVolume *vol = wrapper.volume();

	// Restore previously modified state from flat history array (O(N) sequential iteration,
	// no hash chain traversal). This undoes the previous sculpt so we re-apply from scratch.
	for (const voxel::VoxelPosition &entry : _historyEntries) {
		if (vol->setVoxel(entry.pos, entry.voxel)) {
			wrapper.addToDirtyRegion(entry.pos);
		}
	}
	_historyEntries.clear();

	// Skip sculpt when the mode needs a face direction but none is set yet
	const bool needsFace = modeNeedsFace(_parameters.mode);
	if (needsFace && _parameters.face == voxel::FaceNames::Max) {
		return;
	}

	// Initialize history tracking BitVolume for this evaluate() pass.
	// Sized to snapshot region + margin for modes that grow outward.
	_historyRegion = _snapshotRegion;
	_historyRegion.grow(_parameters.iterations + 2);
	_historyRegion.cropTo(vol->region());
	_historyBits = voxel::BitVolume(_historyRegion);

	applySculpt(wrapper);
}

void SculptOperation::fitPlaneFromSnapshot() {
	core_trace_scoped(SculptBrushFitPlane);

	_planeHeightAxis = math::getIndexForAxis(voxel::faceToAxis(_parameters.face));
	_planeUAxis = (_planeHeightAxis + 1) % 3;
	_planeVAxis = (_planeHeightAxis + 2) % 3;
	const bool fromPositive = voxel::isPositiveFace(_parameters.face);

	// Build height map from snapshot: surface height per (u,v) column
	// Key is ivec3 with height axis zeroed out (no hash<ivec2> available)
	using HeightMap = core::DynamicMap<glm::ivec3, int, 1031, glm::hash<glm::ivec3>>;
	HeightMap heightMap;

	for (const voxel::VoxelPosition &entry : _snapshotEntries) {
		const glm::ivec3 &pos = entry.pos;
		glm::ivec3 key(pos);
		key[_planeHeightAxis] = 0;
		auto it = heightMap.find(key);
		if (it == heightMap.end()) {
			heightMap.put(key, pos[_planeHeightAxis]);
		} else {
			if (fromPositive) {
				it->value = glm::max(it->value, pos[_planeHeightAxis]);
			} else {
				it->value = glm::min(it->value, pos[_planeHeightAxis]);
			}
		}
	}

	if (heightMap.empty()) {
		_planeFitted = false;
		return;
	}

	// Pick seed from first entry
	auto seedIt = heightMap.begin();
	_planeSeedU = seedIt->key[_planeUAxis];
	_planeSeedV = seedIt->key[_planeVAxis];
	_planeSeedH = seedIt->value;

	// 2D linear regression: h = seedH + gradU*(u-seedU) + gradV*(v-seedV)
	double sumDU = 0.0, sumDV = 0.0, sumDH = 0.0;
	double sumDU2 = 0.0, sumDV2 = 0.0, sumDUDV = 0.0;
	double sumDUDH = 0.0, sumDVDH = 0.0;
	int planeN = 0;

	for (auto it = heightMap.begin(); it != heightMap.end(); ++it) {
		const double du = static_cast<double>(it->key[_planeUAxis] - _planeSeedU);
		const double dv = static_cast<double>(it->key[_planeVAxis] - _planeSeedV);
		const double dh = static_cast<double>(it->value - _planeSeedH);
		sumDU += du;
		sumDV += dv;
		sumDH += dh;
		sumDU2 += du * du;
		sumDV2 += dv * dv;
		sumDUDV += du * dv;
		sumDUDH += du * dh;
		sumDVDH += dv * dh;
		++planeN;
	}

	_planeGradU = 0.0f;
	_planeGradV = 0.0f;

	static constexpr int MinPlaneSamples = 3;
	static constexpr double MinDeterminant = 0.001;

	if (planeN >= MinPlaneSamples) {
		const double n = static_cast<double>(planeN);
		const double meanDU = sumDU / n;
		const double meanDV = sumDV / n;
		const double meanDH = sumDH / n;
		const double suu = sumDU2 - n * meanDU * meanDU;
		const double svv = sumDV2 - n * meanDV * meanDV;
		const double suv = sumDUDV - n * meanDU * meanDV;
		const double suh = sumDUDH - n * meanDU * meanDH;
		const double svh = sumDVDH - n * meanDV * meanDH;
		const double det = suu * svv - suv * suv;
		if (glm::abs(det) > MinDeterminant) {
			_planeGradU = static_cast<float>((svv * suh - suv * svh) / det);
			_planeGradV = static_cast<float>((suu * svh - suv * suh) / det);
		}
	}

	_planeFitted = true;
}

void SculptOperation::paintExtendPlane(ModifierVolumeWrapper &wrapper) {
	core_trace_scoped(SculptBrushPaintExtendPlane);

	voxel::RawVolume *vol = wrapper.volume();
	const voxel::Region &volRegion = vol->region();
	const bool fromPositive = voxel::isPositiveFace(_parameters.face);

	const int cursorU = _parameters.cursor[_planeUAxis];
	const int cursorV = _parameters.cursor[_planeVAxis];

	voxel::Voxel fillVoxel = _parameters.voxel;
	if (voxel::isAir(fillVoxel.getMaterial())) {
		fillVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	}
	fillVoxel.setFlags(voxel::FlagOutline);

	// UV neighbor offsets for gap-filling span extension (du, dv)
	static constexpr int uvNeighborDU[] = {-1, 1, 0, 0};
	static constexpr int uvNeighborDV[] = {0, 0, -1, 1};
	static constexpr int numUVNeighbors = 4;

	const voxel::Voxel air;

	for (int u = cursorU - _parameters.radius; u <= cursorU + _parameters.radius; ++u) {
		for (int v = cursorV - _parameters.radius; v <= cursorV + _parameters.radius; ++v) {
			const float predictedHf = static_cast<float>(_planeSeedH) +
									  _planeGradU * static_cast<float>(u - _planeSeedU) +
									  _planeGradV * static_cast<float>(v - _planeSeedV);

			// Fill vertical span from floor to ceil to cover sub-voxel plane position
			int loH = static_cast<int>(glm::floor(predictedHf));
			int hiH = static_cast<int>(glm::ceil(predictedHf));

			// Extend span toward UV neighbors to bridge gaps on steep slopes
			for (int ni = 0; ni < numUVNeighbors; ++ni) {
				const float neighborH = static_cast<float>(_planeSeedH) +
										_planeGradU * static_cast<float>(u + uvNeighborDU[ni] - _planeSeedU) +
										_planeGradV * static_cast<float>(v + uvNeighborDV[ni] - _planeSeedV);
				const int midH = static_cast<int>(glm::round((predictedHf + neighborH) * 0.5f));
				loH = glm::min(loH, midH);
				hiH = glm::max(hiH, midH);
			}

			// In extend-only mode, only place voxels adjacent to existing solid
			bool canPlace = true;
			if (_parameters.extendOnly) {
				canPlace = false;
				for (int h = loH; h <= hiH && !canPlace; ++h) {
					glm::ivec3 pos;
					pos[_planeUAxis] = u;
					pos[_planeVAxis] = v;
					pos[_planeHeightAxis] = h;
					for (const glm::ivec3 &offset : voxel::arrayPathfinderFaces) {
						const glm::ivec3 neighbor = pos + offset;
						if (!volRegion.containsPoint(neighbor)) {
							continue;
						}
						if (voxel::isBlocked(vol->voxel(neighbor).getMaterial())) {
							canPlace = true;
							break;
						}
					}
				}
			}

			// Remove voxels above the plane in this column. Iterate along the
			// height axis from just above the fill span up to removeAboveDepth.
			// Only removes non-outlined voxels so newly placed / selected voxels
			// are preserved. Column-based removal avoids the lateral bleed that
			// the old thick-ray approach caused.
			if (_parameters.removeAboveDepth > 0) {
				const int clearStart = fromPositive ? hiH + 1 : loH - 1;
				const int clearEnd =
					fromPositive ? hiH + _parameters.removeAboveDepth : loH - _parameters.removeAboveDepth;
				const int clearStep = fromPositive ? 1 : -1;
				for (int h = clearStart; fromPositive ? (h <= clearEnd) : (h >= clearEnd); h += clearStep) {
					glm::ivec3 pos;
					pos[_planeUAxis] = u;
					pos[_planeVAxis] = v;
					pos[_planeHeightAxis] = h;
					if (!volRegion.containsPoint(pos)) {
						continue;
					}
					const voxel::Voxel &vx = vol->voxel(pos);
					if (!voxel::isAir(vx.getMaterial()) && (vx.getFlags() & voxel::FlagOutline) == 0) {
						writeVoxel(wrapper, pos, air);
					}
				}
			}

			// Place voxels at predicted plane height. In remove-only mode the brush never
			// places anything - it just carves away the voxels above the plane (handled by
			// the remove-above pass above), so skip the placement entirely.
			if (canPlace && !_parameters.removeOnly) {
				for (int h = loH; h <= hiH; ++h) {
					glm::ivec3 pos;
					pos[_planeUAxis] = u;
					pos[_planeVAxis] = v;
					pos[_planeHeightAxis] = h;

					if (!volRegion.containsPoint(pos)) {
						continue;
					}

					const voxel::Voxel &existing = vol->voxel(pos);
					if (voxel::isAir(existing.getMaterial())) {
						voxel::Voxel newVoxel = fillVoxel;
						for (const glm::ivec3 &offset : voxel::arrayPathfinderFaces) {
							const glm::ivec3 neighbor = pos + offset;
							if (!volRegion.containsPoint(neighbor)) {
								continue;
							}
							const voxel::Voxel &nv = vol->voxel(neighbor);
							if (voxel::isBlocked(nv.getMaterial())) {
								newVoxel = voxel::createVoxel(nv.getMaterial(), nv.getColor(), nv.getNormal(),
															  voxel::FlagOutline, nv.getBoneIdx());
								break;
							}
						}
						writeVoxel(wrapper, pos, newVoxel);
					}
				}
			}
		}
	}
}

} // namespace voxedit
