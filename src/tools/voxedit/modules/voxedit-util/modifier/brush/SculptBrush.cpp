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
	// Clear _active so commit()'s beginBrushFromPanel() can succeed
	_active = false;
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
	_cachedVoxelMapValid = false;
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
	_removeOnly = false;
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
		// Only capture face on first click (when no face is set yet).
		// Subsequent clicks should not re-trigger the full sculpt calculation.
		if (_flattenFace == voxel::FaceNames::Max) {
			_flattenFace = ctx.cursorFace;
			_paramsDirty = true;
		}
		if (_sculptMode == SculptMode::SquashToPlane) {
			const int axisIdx = math::getIndexForAxis(voxel::faceToAxis(_flattenFace));
			_squashPlaneCoord = ctx.cursorPosition[axisIdx];
			_paramsDirty = true;
		}
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

void SculptBrush::generate(scenegraph::SceneGraph &, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
						   const voxel::Region &) {
	if (!_hasSnapshot) {
		return;
	}
	if (!_paramsDirty) {
		return;
	}

	_paramsDirty = false;

	// Fast path for commit: when MarkUndo is set, the volume already has the
	// correct data from the NoUndo preview run. Just report the dirty region
	// from _historyEntries so the undo system records the change.
	if ((_sceneModifiedFlags & SceneModifiedFlags::MarkUndo) == SceneModifiedFlags::MarkUndo) {
		if (!_historyEntries.empty()) {
			for (const voxel::VoxelPosition &entry : _historyEntries) {
				wrapper.addToDirtyRegion(entry.pos);
			}
		}
		return;
	}

	// ExtendPlane uses additive painting - no history restore between strokes
	if (_sculptMode == SculptMode::ExtendPlane) {
		if (!_planeFitted && _flattenFace != voxel::FaceNames::Max) {
			fitPlaneFromSnapshot();
			if (_planeFitted) {
				// Initialize history tracking for dedup in saveToHistory().
				// Must happen before any painting so the first write to each position
				// is recorded as the original value.
				voxel::RawVolume *vol = wrapper.volume();
				_historyRegion = _snapshotRegion;
				_historyRegion.grow(_brushRadius + _removeAboveDepth + 2);
				_historyRegion.cropTo(vol->region());
				_historyBits = voxel::BitVolume(_historyRegion);
			}
		}
		if (_planeFitted && _active) {
			paintExtendPlane(wrapper, ctx.cursorVoxel, ctx.cursorPosition);
			_lastPaintPos = ctx.cursorPosition;
			markDirty();
		} else if (_planeFitted && !_active && !_historyEntries.empty()) {
			// Commit path: _active is false, changes are already in the volume.
			// Mark all modified positions as dirty so the undo system captures them.
			for (const voxel::VoxelPosition &entry : _historyEntries) {
				wrapper.addToDirtyRegion(entry.pos);
			}
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

	applySculpt(wrapper, ctx.cursorVoxel);
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
		// Auto-detect depth axis from thinnest dimension (positive face = outward)
		if (extents.x <= extents.y && extents.x <= extents.z) {
			_reskinConfig.skinFace = voxel::FaceNames::PositiveX;
		} else if (extents.z <= extents.x && extents.z <= extents.y) {
			_reskinConfig.skinFace = voxel::FaceNames::PositiveZ;
		} else {
			_reskinConfig.skinFace = voxel::FaceNames::PositiveY;
		}
		const int upIdx = math::getIndexForAxis(voxel::faceToAxis(_reskinConfig.skinFace));
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
