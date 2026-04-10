/**
 * @file
 */

#include "SculptBrush.h"
#include "core/GLM.h"
#include "core/Trace.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/DynamicMap.h"
#include "math/Axis.h"
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
	_planeFitted = false;
	_lastPaintPos = glm::ivec3(INT_MIN);
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
	// Force re-apply so generate() runs and creates a dirty region for the undo entry
	_paramsDirty = true;
	return hasPendingChanges();
}

void SculptBrush::reset() {
	Super::reset();
	_sceneModifiedFlags = SceneModifiedFlags::All;
	_active = false;
	_paramsDirty = true;
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
	_kernelSize = 4;
	_sigma = 4.0f;
	_sculptMode = SculptMode::Erode;
	_flattenFace = voxel::FaceNames::Max;
	_smoothWallClearDepth = MaxSmoothWallClearDepth;
	_smoothWallInterp = voxelutil::SmoothWallInterp::InverseDistance;
	_smoothWallFillHoles = true;
	_removeAboveDepth = 0;
	_extendOnly = true;
	_brushRadius = 3;
	_planeFitted = false;
	_planeGradU = 0.0f;
	_planeGradV = 0.0f;
	_lastPaintPos = glm::ivec3(INT_MIN);
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
		captureSnapshot(volume, ctx.targetVolumeRegion);
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
	return ctx.targetVolumeRegion;
}

void SculptBrush::captureSnapshot(const voxel::RawVolume *volume, const voxel::Region &volRegion) {
	core_trace_scoped(SculptBrushCaptureSnapshot);
	_snapshot.clear();
	glm::ivec3 selLo(volRegion.getUpperCorner());
	glm::ivec3 selHi(volRegion.getLowerCorner());

	voxelutil::visitVolume(
		*volume, volRegion,
		[&](int x, int y, int z, const voxel::Voxel &voxel) {
			const glm::ivec3 pos(x, y, z);
			_snapshot.setVoxel(pos, voxel);
			selLo = glm::min(selLo, pos);
			selHi = glm::max(selHi, pos);
		},
		voxelutil::VisitSolidOutline());

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
	} else if (_sculptMode == SculptMode::SmoothWall && _flattenFace != voxel::FaceNames::Max) {
		voxel::Voxel fillVoxel = ctx.cursorVoxel;
		fillVoxel.setFlags(voxel::FlagOutline);
		static constexpr int smoothWallIterations = 1;
		voxelutil::sculptSmoothWall(currentSolid, voxelMap, anchorSolid, _flattenFace, smoothWallIterations,
									fillVoxel, _smoothWallClearDepth, _smoothWallInterp, _smoothWallFillHoles);
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

	struct HeightCollector {
		HeightMap *map;
		int uAxis;
		int vAxis;
		int heightAxis;
		bool fromPositive;
		bool setVoxel(int x, int y, int z, const voxel::Voxel &) {
			const glm::ivec3 pos(x, y, z);
			glm::ivec3 key(pos);
			key[heightAxis] = 0;
			auto it = map->find(key);
			if (it == map->end()) {
				map->put(key, pos[heightAxis]);
			} else {
				if (fromPositive) {
					it->value = glm::max(it->value, pos[heightAxis]);
				} else {
					it->value = glm::min(it->value, pos[heightAxis]);
				}
			}
			return true;
		}
	};
	HeightCollector collector{&heightMap, _planeUAxis, _planeVAxis, _planeHeightAxis, fromPositive};
	_snapshot.copyTo(collector);

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

void SculptBrush::setReskinSkinUpAxis(math::Axis axis) {
	_reskinConfig.skinUpAxis = axis;
	// Re-populate skin depth for the new axis
	if (_skinVolume != nullptr) {
		setSkinVolume(_skinVolume);
	}
	_paramsDirty = true;
}

void SculptBrush::setSkinVolume(const voxel::RawVolume *skinVolume) {
	_skinVolume = skinVolume;
	// Auto-populate skin depth from the skin volume's depth along the configured up axis
	if (skinVolume != nullptr) {
		const voxel::Region &sr = skinVolume->region();
		const int upIdx = math::getIndexForAxis(_reskinConfig.skinUpAxis);
		const int depthExtent = sr.getUpperCorner()[upIdx] - sr.getLowerCorner()[upIdx] + 1;
		_reskinConfig.skinDepth = glm::clamp(depthExtent, 1, MaxReskinDepth);
	}
	_paramsDirty = true;
}

void SculptBrush::setOwnedSkinVolume(voxel::RawVolume *skinVolume, const core::String &filePath) {
	_ownedSkinVolume = skinVolume;
	_skinFilePath = filePath;
	setSkinVolume(skinVolume);
}

} // namespace voxedit
