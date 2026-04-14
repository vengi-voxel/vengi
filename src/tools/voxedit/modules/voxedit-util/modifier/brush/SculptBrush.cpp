/**
 * @file
 */

#include "SculptBrush.h"
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

namespace voxedit {

void SculptBrush::onSceneChange() {
	Super::onSceneChange();
	_active = false;
	_hasSnapshot = false;
	_snapshotEntries.clear();
	_historyEntries.clear();
	_historyRegion = voxel::Region::InvalidRegion;
	_snapshotRegion = voxel::Region::InvalidRegion;
	_cachedRegion = voxel::Region::InvalidRegion;
	_cachedRegionValid = false;
	_planeFitted = false;
	_lastPaintPos = glm::ivec3(INT_MIN);
}

void SculptBrush::onActivated() {
	reset();
	// Suppress undo registration during preview - only the final commit should create an undo entry.
	// Also skip InvalidateNodeCache: selection bounding box does not change during sculpt preview,
	// and regionForFlag() scans the full volume which is very expensive on large models.
	_sceneModifiedFlags = SceneModifiedFlags::NoUndo & ~SceneModifiedFlags::InvalidateNodeCache;
}

bool SculptBrush::hasPendingChanges() const {
	return _hasSnapshot;
}

voxel::Region SculptBrush::revertChanges(voxel::RawVolume *volume) {
	voxel::RawVolumeWrapper wrapper(volume);
	for (const voxel::VoxelPosition &entry : _historyEntries) {
		wrapper.setVoxel(entry.pos.x, entry.pos.y, entry.pos.z, entry.voxel);
	}
	_historyEntries.clear();
	_historyRegion = voxel::Region::InvalidRegion;
	return wrapper.dirtyRegion();
}

bool SculptBrush::onDeactivated() {
	// Restore undo registration so the final execute in setBrushType() records the undo entry
	_sceneModifiedFlags = SceneModifiedFlags::All;
	// Force re-apply so generate() runs and creates a dirty region for the undo entry
	_paramsDirty = true;
	return hasPendingChanges();
}

void SculptBrush::reset() {
	Super::reset();
	_sceneModifiedFlags = SceneModifiedFlags::All;
	_active = false;
	_paramsDirty = false;
	_hasSnapshot = false;
	_snapshotEntries.clear();
	_historyEntries.clear();
	_historyRegion = voxel::Region::InvalidRegion;
	_cachedBitVolumeValid = false;
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
	_removeAboveDepth = 0;
	_extendOnly = true;
	_brushRadius = 3;
	_planeFitted = false;
	_planeGradU = 0.0f;
	_planeGradV = 0.0f;
	_lastPaintPos = glm::ivec3(INT_MIN);
	_reskinConfig.preview = true;
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
	_lastPaintPos = glm::ivec3(INT_MIN);
}

bool SculptBrush::active() const {
	return _active || _hasSnapshot;
}

void SculptBrush::preExecute(const BrushContext &ctx, const voxel::RawVolume *volume) {
	if (!_hasSnapshot && volume != nullptr) {
		// Defer snapshot capture until there is actually work to do.
		if (_paramsDirty) {
			captureSnapshot(volume, ctx.targetVolumeRegion);
		}
	} else if (_hasSnapshot) {
		const glm::ivec3 delta = ctx.targetVolumeRegion.getLowerCorner() - _capturedVolumeLower;
		if (delta != glm::ivec3(0)) {
			adjustSnapshotForRegionShift(delta);
		}
	}
	if (_hasSnapshot && _sculptMode == SculptMode::ExtendPlane && _planeFitted) {
		if (_active && ctx.cursorPosition != _lastPaintPos) {
			_paramsDirty = true;
		}
	}
	if (_hasSnapshot) {
		_cachedRegion = _snapshotRegion;
		if (_sculptMode == SculptMode::Reskin) {
			// Expand along face normal for z offset + skin layers growing outward
			const int expand = glm::abs(_reskinConfig.zOffset) + _reskinConfig.skinDepth;
			_cachedRegion.grow(expand);
		} else if (_sculptMode == SculptMode::ExtendPlane && _planeFitted) {
			// Show brush preview around cursor position
			const int r = _brushRadius;
			glm::ivec3 lo = ctx.cursorPosition;
			glm::ivec3 hi = ctx.cursorPosition;
			lo[_planeUAxis] -= r;
			lo[_planeVAxis] -= r;
			hi[_planeUAxis] += r;
			hi[_planeVAxis] += r;
			// Predict height range across the brush corners
			const float h00 = static_cast<float>(_planeSeedH) +
							  _planeGradU * static_cast<float>(lo[_planeUAxis] - _planeSeedU) +
							  _planeGradV * static_cast<float>(lo[_planeVAxis] - _planeSeedV);
			const float h11 = static_cast<float>(_planeSeedH) +
							  _planeGradU * static_cast<float>(hi[_planeUAxis] - _planeSeedU) +
							  _planeGradV * static_cast<float>(hi[_planeVAxis] - _planeSeedV);
			const float h01 = static_cast<float>(_planeSeedH) +
							  _planeGradU * static_cast<float>(lo[_planeUAxis] - _planeSeedU) +
							  _planeGradV * static_cast<float>(hi[_planeVAxis] - _planeSeedV);
			const float h10 = static_cast<float>(_planeSeedH) +
							  _planeGradU * static_cast<float>(hi[_planeUAxis] - _planeSeedU) +
							  _planeGradV * static_cast<float>(lo[_planeVAxis] - _planeSeedV);
			const float minH = glm::min(glm::min(h00, h11), glm::min(h01, h10));
			const float maxH = glm::max(glm::max(h00, h11), glm::max(h01, h10));
			lo[_planeHeightAxis] = static_cast<int>(glm::floor(minH)) - 1;
			hi[_planeHeightAxis] = static_cast<int>(glm::ceil(maxH)) + 1;
			_cachedRegion = voxel::Region(lo, hi);
		} else if (_sculptMode == SculptMode::ExtendPlane) {
			// Plane not yet fitted - show snapshot region
			_cachedRegion.grow(_iterations);
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
	if (!_hasSnapshot) {
		return voxel::Region::InvalidRegion;
	}
	return ctx.targetVolumeRegion;
}

void SculptBrush::captureSnapshot(const voxel::RawVolume *volume, const voxel::Region &volRegion) {
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
}

void SculptBrush::adjustSnapshotForRegionShift(const glm::ivec3 &delta) {
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
}

void SculptBrush::saveToHistory(voxel::RawVolume *vol, const glm::ivec3 &pos) {
	// O(1) bit test instead of O(N/buckets) hash chain traversal
	if (_historyRegion.containsPoint(pos) && _historyBits.hasValue(pos.x, pos.y, pos.z)) {
		return;
	}
	if (_historyRegion.containsPoint(pos)) {
		_historyBits.setVoxel(pos.x, pos.y, pos.z, true);
	}
	_historyEntries.push_back({pos, vol->voxel(pos)});
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

void SculptBrush::rebuildCachedBitVolume() {
	if (_cachedBitVolumeValid) {
		return;
	}
	core_trace_scoped(SculptBrushRebuildBitVolume);
	_cachedBitVolume = voxel::BitVolume(_snapshotRegion);
	for (const voxel::VoxelPosition &entry : _snapshotEntries) {
		_cachedBitVolume.setVoxel(entry.pos.x, entry.pos.y, entry.pos.z, true);
	}
	_cachedBitVolumeValid = true;
}

void SculptBrush::applySculpt(ModifierVolumeWrapper &wrapper, const BrushContext &ctx) {
	core_trace_scoped(SculptBrushApplySculpt);

	// Ensure the cached BitVolume is up to date (built from flat _snapshotEntries).
	rebuildCachedBitVolume();

	// ---- Reskin fast path ----
	// sculptReskin never reads from voxelMap, only writes. So we skip the expensive O(N)
	// SparseVolume construction and full write-back. Instead: copy cached BitVolume (cheap
	// bit memcpy), pass empty voxelMap, and write back only the entries sculptReskin modified.
	// ---- Reskin: early return when no skin loaded ----
	if (_sculptMode == SculptMode::Reskin && _skinVolume == nullptr) {
		return;
	}

	if (_sculptMode == SculptMode::Reskin && _skinVolume != nullptr && _flattenFace != voxel::FaceNames::Max) {
		voxel::BitVolume workSolid(_cachedBitVolume);
		voxel::SparseVolume voxelMap;

		const palette::Palette *skinPal = skinPalette();
		palette::Palette &nodePal = wrapper.node().palette();
		voxelutil::sculptReskin(workSolid, voxelMap, *_skinVolume, _flattenFace, _reskinConfig,
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

	// ---- Generic path for all other sculpt modes ----
	voxel::Region workRegion = _snapshotRegion;

	// Erode only writes air to removed positions - it never reads voxel data from voxelMap.
	// Skip the expensive O(N) SparseVolume construction for it.
	const bool needsVoxelMap = _sculptMode != SculptMode::Erode;

	// Build BitVolume from cached entries. Only build SparseVolume when the mode needs it.
	voxel::BitVolume currentSolid(workRegion);
	voxel::SparseVolume voxelMap;
	for (const voxel::VoxelPosition &entry : _snapshotEntries) {
		currentSolid.setVoxel(entry.pos.x, entry.pos.y, entry.pos.z, true);
	}
	if (needsVoxelMap) {
		for (const voxel::VoxelPosition &entry : _snapshotEntries) {
			voxelMap.setVoxel(entry.pos, entry.voxel);
		}
	}

	voxel::RawVolume *vol = wrapper.volume();
	const voxel::Region &volRegion = vol->region();

	// Build anchor set only for modes that use it
	const bool needsAnchors = _sculptMode == SculptMode::Erode || _sculptMode == SculptMode::Grow ||
							  _sculptMode == SculptMode::SmoothAdditive || _sculptMode == SculptMode::SmoothErode ||
							  _sculptMode == SculptMode::SmoothGaussian || _sculptMode == SculptMode::BridgeGap;
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
			if (mapVoxel.getColor() != entry.voxel.getColor() ||
				mapVoxel.getMaterial() != entry.voxel.getMaterial() ||
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

	// ExtendPlane uses additive painting - no history restore between strokes
	if (_sculptMode == SculptMode::ExtendPlane) {
		if (!_planeFitted && _flattenFace != voxel::FaceNames::Max) {
			fitPlaneFromSnapshot();
		}
		if (_planeFitted && _active) {
			paintExtendPlane(wrapper, ctx);
			_lastPaintPos = ctx.cursorPosition;
			markDirty();
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
	const bool needsFace = modeNeedsFace(_sculptMode);
	if (needsFace && _flattenFace == voxel::FaceNames::Max) {
		return;
	}

	// Initialize history tracking BitVolume for this generate() pass.
	// Sized to snapshot region + margin for modes that grow outward.
	_historyRegion = _snapshotRegion;
	_historyRegion.grow(_iterations + 2);
	_historyRegion.cropTo(vol->region());
	_historyBits = voxel::BitVolume(_historyRegion);

	applySculpt(wrapper, ctx);
}

void SculptBrush::fitPlaneFromSnapshot() {
	core_trace_scoped(SculptBrushFitPlane);

	_planeHeightAxis = math::getIndexForAxis(voxel::faceToAxis(_flattenFace));
	_planeUAxis = (_planeHeightAxis + 1) % 3;
	_planeVAxis = (_planeHeightAxis + 2) % 3;
	const bool fromPositive = voxel::isPositiveFace(_flattenFace);

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

void SculptBrush::paintExtendPlane(ModifierVolumeWrapper &wrapper, const BrushContext &ctx) {
	core_trace_scoped(SculptBrushPaintExtendPlane);

	voxel::RawVolume *vol = wrapper.volume();
	const voxel::Region &volRegion = vol->region();
	const bool fromPositive = voxel::isPositiveFace(_flattenFace);
	const int step = fromPositive ? 1 : -1;

	const int cursorU = ctx.cursorPosition[_planeUAxis];
	const int cursorV = ctx.cursorPosition[_planeVAxis];

	voxel::Voxel fillVoxel = ctx.cursorVoxel;
	if (voxel::isAir(fillVoxel.getMaterial())) {
		fillVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	}
	fillVoxel.setFlags(voxel::FlagOutline);

	// UV neighbor offsets for gap-filling span extension (du, dv)
	static constexpr int uvNeighborDU[] = {-1, 1, 0, 0};
	static constexpr int uvNeighborDV[] = {0, 0, -1, 1};
	static constexpr int numUVNeighbors = 4;

	// Precompute removal ray direction (perpendicular to fitted plane)
	static constexpr float RayStep = 0.5f;
	const voxel::Voxel air;
	glm::vec3 removeNormal(0.0f);
	const float removeLineLen = static_cast<float>(_removeAboveDepth);
	if (_removeAboveDepth > 0) {
		removeNormal[_planeUAxis] = -_planeGradU;
		removeNormal[_planeVAxis] = -_planeGradV;
		removeNormal[_planeHeightAxis] = 1.0f;
		removeNormal *= static_cast<float>(step);
		removeNormal = glm::normalize(removeNormal);
	}

	for (int u = cursorU - _brushRadius; u <= cursorU + _brushRadius; ++u) {
		for (int v = cursorV - _brushRadius; v <= cursorV + _brushRadius; ++v) {
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
			if (_extendOnly) {
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

			// Remove voxels along a thick 3D ray perpendicular to the plane.
			// Only remove voxels that do NOT have FlagOutline (preserves selected
			// and newly placed voxels).
			if (_removeAboveDepth > 0) {
				glm::vec3 origin;
				origin[_planeUAxis] = static_cast<float>(u);
				origin[_planeVAxis] = static_cast<float>(v);
				origin[_planeHeightAxis] = static_cast<float>(hiH) + 0.5f * static_cast<float>(step);

				glm::ivec3 prevPos(INT_MIN);
				for (float t = 0.0f; t <= removeLineLen; t += RayStep) {
					const glm::vec3 fp = origin + removeNormal * t;
					const glm::ivec3 pos(static_cast<int>(glm::round(fp.x)),
										 static_cast<int>(glm::round(fp.y)),
										 static_cast<int>(glm::round(fp.z)));
					if (pos == prevPos) {
						continue;
					}
					prevPos = pos;
					// Clear center + 18 face/edge neighbors, but skip outlined voxels
					if (volRegion.containsPoint(pos)) {
						const voxel::Voxel &vx = vol->voxel(pos);
						if (!voxel::isAir(vx.getMaterial()) && (vx.getFlags() & voxel::FlagOutline) == 0) {
							writeVoxel(wrapper, pos, air);
						}
					}
					for (const glm::ivec3 &offset : voxel::arrayPathfinderFaces) {
						const glm::ivec3 nPos = pos + offset;
						if (volRegion.containsPoint(nPos)) {
							const voxel::Voxel &vx = vol->voxel(nPos);
							if (!voxel::isAir(vx.getMaterial()) && (vx.getFlags() & voxel::FlagOutline) == 0) {
								writeVoxel(wrapper, nPos, air);
							}
						}
					}
					for (const glm::ivec3 &offset : voxel::arrayPathfinderEdges) {
						const glm::ivec3 nPos = pos + offset;
						if (volRegion.containsPoint(nPos)) {
							const voxel::Voxel &vx = vol->voxel(nPos);
							if (!voxel::isAir(vx.getMaterial()) && (vx.getFlags() & voxel::FlagOutline) == 0) {
								writeVoxel(wrapper, nPos, air);
							}
						}
					}
				}
			}

			// Place voxels at predicted plane height
			if (canPlace) {
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

void SculptBrush::setSculptMode(SculptMode mode) {
	const bool needsFace = modeNeedsFace(mode);
	const bool hadFace = modeNeedsFace(_sculptMode);
	if (needsFace && !hadFace) {
		_flattenFace = voxel::FaceNames::Max;
	}
	if (mode == SculptMode::SmoothGaussian && _sculptMode != SculptMode::SmoothGaussian) {
		_iterations = 3;
	}
	if (mode == SculptMode::ExtendPlane && _sculptMode != SculptMode::ExtendPlane) {
		_planeFitted = false;
		_lastPaintPos = glm::ivec3(INT_MIN);
	}
	_sculptMode = mode;
	_paramsDirty = true;
}

void SculptBrush::setSkinVolume(const voxel::RawVolume *skinVolume) {
	_skinVolume = skinVolume;
	if (skinVolume != nullptr) {
		const voxel::Region &sr = skinVolume->region();
		const glm::ivec3 extents = sr.getUpperCorner() - sr.getLowerCorner() + 1;
		// Auto-detect depth axis from thinnest dimension
		if (extents.x <= extents.y && extents.x <= extents.z) {
			_reskinConfig.skinDepthAxis = math::Axis::X;
		} else if (extents.z <= extents.x && extents.z <= extents.y) {
			_reskinConfig.skinDepthAxis = math::Axis::Z;
		} else {
			_reskinConfig.skinDepthAxis = math::Axis::Y;
		}
		const int upIdx = math::getIndexForAxis(_reskinConfig.skinDepthAxis);
		const int depthExtent = extents[upIdx];
		_reskinConfig.skinDepth = glm::clamp(depthExtent, 1, MaxReskinDepth);
	}
	_paramsDirty = true;
}

void SculptBrush::setOwnedSkinVolume(voxel::RawVolume *skinVolume, const core::String &filePath,
									 const palette::Palette *skinPalette) {
	_ownedSkinVolume = skinVolume;
	_skinFilePath = filePath;
	if (skinPalette != nullptr) {
		_skinPalette = *skinPalette;
	} else {
		_skinPalette = {};
	}
	setSkinVolume(skinVolume);
}

} // namespace voxedit
