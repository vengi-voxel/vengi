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

SculptBrush::SculptBrush(const SculptBrush &other) : Super(other), SculptOperation() {
	setParameters(other.parameters());
	_skinSourceIdentity = _parameters.skin.get();
	_skinFilePath = other._skinFilePath;
}

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
	clearOperation();
}

void SculptBrush::reset() {
	Super::reset();
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
	_parameters.strength = 0.5f;
	_parameters.iterations = 1;
	_parameters.heightThreshold = 1;
	_parameters.preserveTopHeight = false;
	_parameters.trimPerStep = 1;
	_parameters.kernelSize = 4;
	_parameters.sigma = 4.0f;
	_parameters.mode = SculptMode::Erode;
	_parameters.face = voxel::FaceNames::Max;
	_parameters.smoothWallClearDepth = 256;
	_parameters.smoothWallInterp = voxelutil::SmoothWallInterp::InverseDistance;
	_parameters.smoothWallFillHoles = true;
	_parameters.removeAboveDepth = 0;
	_parameters.extendOnly = true;
	_parameters.removeOnly = false;
	_parameters.radius = 3;
	_planeFitted = false;
	_planeGradU = 0.0f;
	_planeGradV = 0.0f;
	_lastPaintPos = glm::ivec3(INT_MIN);
	_parameters.reskin.preview = true;
}

bool SculptBrush::beginBrush(const BrushContext &ctx) {
	if (_active) {
		return false;
	}
	const bool needsFace = modeNeedsFace(_parameters.mode);
	if (needsFace && ctx.cursorFace != voxel::FaceNames::Max) {
		// Only capture face on first click (when no face is set yet).
		// Subsequent clicks should not re-trigger the full sculpt calculation.
		if (_parameters.face == voxel::FaceNames::Max) {
			_parameters.face = ctx.cursorFace;
			_paramsDirty = true;
			markDirty();
		}
		if (_parameters.mode == SculptMode::SquashToPlane) {
			const int axisIdx = math::getIndexForAxis(voxel::faceToAxis(_parameters.face));
			_parameters.plane = ctx.cursorPosition[axisIdx];
			_paramsDirty = true;
			markDirty();
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
	return _parameters.mode == SculptMode::ExtendPlane || _active || _hasSnapshot;
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
	if (_hasSnapshot && _parameters.mode == SculptMode::ExtendPlane && !_planeFitted &&
		_parameters.face != voxel::FaceNames::Max) {
		fitPlaneFromSnapshot();
	}
	if (_hasSnapshot && _parameters.mode == SculptMode::ExtendPlane && _planeFitted) {
		if (_active && ctx.cursorPosition != _lastPaintPos) {
			_paramsDirty = true;
			markDirty();
		}
	}
	if (_hasSnapshot) {
		_cachedRegion = _snapshotRegion;
		if (_parameters.mode == SculptMode::Reskin) {
			// Expand along face normal for z offset + skin layers growing outward
			const int expand = glm::abs(_parameters.reskin.zOffset) + _parameters.reskin.skinDepth;
			_cachedRegion.grow(expand);
		} else if (_parameters.mode == SculptMode::ExtendPlane && _planeFitted) {
			// Show brush preview around cursor position
			const int r = _parameters.radius;
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
		} else if (_parameters.mode == SculptMode::ExtendPlane) {
			// Plane not yet fitted - show snapshot region
			_cachedRegion.grow(_parameters.iterations);
		} else {
			// Expand by iterations since smoothing can grow surface by 1 voxel per iteration
			_cachedRegion.grow(_parameters.iterations);
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
	_parameters.voxel = ctx.cursorVoxel;
	_parameters.cursor = ctx.cursorPosition;
	evaluate(wrapper);
}

void SculptBrush::setSculptMode(SculptMode mode) {
	markSettingsChanged();
	const bool needsFace = modeNeedsFace(mode);
	const bool hadFace = modeNeedsFace(_parameters.mode);
	if (needsFace && !hadFace) {
		_parameters.face = voxel::FaceNames::Max;
	}
	if (mode == SculptMode::SmoothGaussian && _parameters.mode != SculptMode::SmoothGaussian) {
		_parameters.iterations = 3;
	}
	if (mode == SculptMode::ExtendPlane && _parameters.mode != SculptMode::ExtendPlane) {
		_planeFitted = false;
		_lastPaintPos = glm::ivec3(INT_MIN);
	}
	_parameters.mode = mode;
	_paramsDirty = true;
	markDirty();
}

void SculptBrush::setSkinVolume(const voxel::RawVolume *skinVolume) {
	markSettingsChanged();
	_skinSourceIdentity = skinVolume;
	_parameters.skin =
		skinVolume ? core::make_shared<voxel::RawVolume>(*skinVolume) : core::SharedPtr<voxel::RawVolume>();
	if (skinVolume != nullptr) {
		const voxel::Region &sr = skinVolume->region();
		const glm::ivec3 extents = sr.getUpperCorner() - sr.getLowerCorner() + 1;
		// Auto-detect depth axis from thinnest dimension (positive face = outward)
		if (extents.x <= extents.y && extents.x <= extents.z) {
			_parameters.reskin.skinFace = voxel::FaceNames::PositiveX;
		} else if (extents.z <= extents.x && extents.z <= extents.y) {
			_parameters.reskin.skinFace = voxel::FaceNames::PositiveZ;
		} else {
			_parameters.reskin.skinFace = voxel::FaceNames::PositiveY;
		}
		const int upIdx = math::getIndexForAxis(voxel::faceToAxis(_parameters.reskin.skinFace));
		const int depthExtent = extents[upIdx];
		_parameters.reskin.skinDepth = glm::clamp(depthExtent, 1, MaxReskinDepth);
	}
	_paramsDirty = true;
	markDirty();
}

void SculptBrush::setOwnedSkinVolume(voxel::RawVolume *skinVolume, const core::String &filePath,
									 const palette::Palette *skinPalette) {
	markSettingsChanged();
	_ownedSkinVolume = skinVolume;
	_skinFilePath = filePath;
	if (skinPalette != nullptr) {
		_parameters.skinPalette = *skinPalette;
	} else {
		_parameters.skinPalette = {};
	}
	setSkinVolume(skinVolume);
}

} // namespace voxedit
