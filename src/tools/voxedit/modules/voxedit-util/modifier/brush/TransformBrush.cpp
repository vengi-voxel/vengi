/**
 * @file
 */

#include "TransformBrush.h"
#include "math/Axis.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/Connectivity.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeRescaler.h"
#include "voxelutil/VolumeRotator.h"
#include "voxelutil/VolumeVisitor.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/trigonometric.hpp>
#include <glm/common.hpp>

namespace voxedit {

void TransformBrush::onSceneChange() {
	Super::onSceneChange();
	_active = false;
	_hasSnapshot = false;
	_snapshotVolume.clear();
	_historyVolume.clear();
	_snapshotRegion = voxel::Region::InvalidRegion;
	_cachedRegion = voxel::Region::InvalidRegion;
	_cachedRegionValid = false;
	_moveOffset = glm::ivec3(0);
	_shearOffset = glm::ivec3(0);
	_scale = glm::vec3(1.0f);
	_rotationDegrees = glm::vec3(0.0f);
}

void TransformBrush::reset() {
	Super::reset();
	_active = false;
	_hasSnapshot = false;
	_snapshotVolume.clear();
	_historyVolume.clear();
	_snapshotRegion = voxel::Region::InvalidRegion;
	_cachedRegion = voxel::Region::InvalidRegion;
	_cachedRegionValid = false;
	_snapshotCenter = glm::vec3(0.0f);
	_moveOffset = glm::ivec3(0);
	_shearOffset = glm::ivec3(0);
	_scale = glm::vec3(1.0f);
	_rotationDegrees = glm::vec3(0.0f);
	_transformMode = TransformMode::Move;
	_scaleSampling = voxelutil::ScaleSampling::Nearest;
}

bool TransformBrush::beginBrush(const BrushContext &ctx) {
	if (_active) {
		return false;
	}
	_active = true;
	return true;
}

void TransformBrush::endBrush(BrushContext &) {
	_active = false;
}

bool TransformBrush::active() const {
	return _active || _hasSnapshot;
}

void TransformBrush::preExecute(const BrushContext &ctx, const voxel::RawVolume *volume) {
	if (!_hasSnapshot && volume != nullptr) {
		captureSnapshot(volume, ctx.targetVolumeRegion);
	}
	_cachedRegion = computeTransformedRegion();
	_cachedRegionValid = _cachedRegion.isValid();
}

voxel::Region TransformBrush::calcRegion(const BrushContext &ctx) const {
	if (_cachedRegionValid) {
		return _cachedRegion;
	}
	return ctx.targetVolumeRegion;
}

void TransformBrush::captureSnapshot(const voxel::RawVolume *volume, const voxel::Region &volRegion) {
	_snapshotVolume.clear();

	auto func = [this](int x, int y, int z, const voxel::Voxel &voxel) {
		if (!(voxel.getFlags() & voxel::FlagOutline)) {
			return;
		}
		_snapshotVolume.setVoxel(x, y, z, voxel);
	};
	voxelutil::visitVolumeParallel(*volume, volRegion, func);

	if (_snapshotVolume.empty()) {
		_hasSnapshot = false;
		return;
	}

	_snapshotRegion = _snapshotVolume.calculateRegion();
	_snapshotCenter = glm::vec3(_snapshotRegion.getLowerCorner() + _snapshotRegion.getUpperCorner()) * 0.5f;
	_hasSnapshot = true;
}

voxel::Region TransformBrush::computeTransformedRegion() const {
	if (!_hasSnapshot || _snapshotVolume.empty()) {
		return voxel::Region::InvalidRegion;
	}
	const glm::ivec3 &srcLo = _snapshotRegion.getLowerCorner();
	const glm::ivec3 &srcHi = _snapshotRegion.getUpperCorner();
	glm::ivec3 dstLo = srcLo;
	glm::ivec3 dstHi = srcHi;

	static constexpr int NumCorners = 8;
	for (int corner = 0; corner < NumCorners; ++corner) {
		const glm::ivec3 cornerPos(
			(corner & 1) ? srcHi.x : srcLo.x,
			(corner & 2) ? srcHi.y : srcLo.y,
			(corner & 4) ? srcHi.z : srcLo.z);
		const glm::ivec3 transformed = transformPosition(cornerPos);
		dstLo = glm::min(dstLo, transformed);
		dstHi = glm::max(dstHi, transformed);
	}

	// Union of original and transformed regions with a small margin for rounding
	static constexpr int Margin = 2;
	const glm::ivec3 unionLo = glm::min(srcLo, dstLo) - Margin;
	const glm::ivec3 unionHi = glm::max(srcHi, dstHi) + Margin;
	return voxel::Region(unionLo, unionHi);
}

glm::ivec3 TransformBrush::transformPosition(const glm::ivec3 &pos) const {
	const glm::vec3 center = _snapshotCenter;
	glm::vec3 result(pos);

	switch (_transformMode) {
	case TransformMode::Move:
		result += glm::vec3(_moveOffset);
		break;

	case TransformMode::Shear: {
		// Shear transform: each slice along a perpendicular axis shifts proportionally
		// to its distance from the center - like pushing the top of a bread stack sideways.
		// X shear: each Y-layer shifts in X (top goes +X, bottom goes -X)
		// Y shear: each Z-layer shifts in Y (front goes +Y, back goes -Y)
		// Z shear: each X-layer shifts in Z (right goes +Z, left goes -Z)
		const glm::vec3 relative = result - center;
		const glm::vec3 halfSize = glm::vec3(_snapshotRegion.getDimensionsInVoxels()) * 0.5f;

		if (halfSize.y > 0.0f && _shearOffset.x != 0) {
			result.x += (relative.y / halfSize.y) * (float)_shearOffset.x;
		}
		if (halfSize.z > 0.0f && _shearOffset.y != 0) {
			result.y += (relative.z / halfSize.z) * (float)_shearOffset.y;
		}
		if (halfSize.x > 0.0f && _shearOffset.z != 0) {
			result.z += (relative.x / halfSize.x) * (float)_shearOffset.z;
		}
		break;
	}

	case TransformMode::Scale: {
		const glm::vec3 relative = result - center;
		result = center + relative * _scale;
		break;
	}

	case TransformMode::Rotate: {
		glm::vec3 relative = result - center;
		if (_rotationDegrees.x != 0.0f) {
			relative = glm::rotateX(relative, glm::radians(_rotationDegrees.x));
		}
		if (_rotationDegrees.y != 0.0f) {
			relative = glm::rotateY(relative, glm::radians(_rotationDegrees.y));
		}
		if (_rotationDegrees.z != 0.0f) {
			relative = glm::rotateZ(relative, glm::radians(_rotationDegrees.z));
		}
		result = center + relative;
		break;
	}

	default:
		break;
	}

	return glm::ivec3(glm::round(result));
}

void TransformBrush::saveToHistory(voxel::RawVolume *vol, const glm::ivec3 &pos) {
	if (_historyVolume.hasVoxel(pos)) {
		return;
	}
	_historyVolume.setVoxel(pos, vol->voxel(pos));
}

void TransformBrush::writeVoxel(ModifierVolumeWrapper &wrapper, const glm::ivec3 &pos, const voxel::Voxel &newVoxel) {
	voxel::RawVolume *vol = wrapper.volume();
	if (!vol->region().containsPoint(pos)) {
		return;
	}
	saveToHistory(vol, pos);
	if (vol->setVoxel(pos, newVoxel)) {
		wrapper.addToDirtyRegion(pos);
	}
}

void TransformBrush::applyTransform(ModifierVolumeWrapper &wrapper, const BrushContext &ctx) {
	voxel::RawVolume *vol = wrapper.volume();
	const voxel::Region &volRegion = vol->region();
	const voxel::Voxel air;

	// Erase all original selected voxels (saving to history)
	auto funcErase = [&](int x, int y, int z, const voxel::Voxel &) {
		if (volRegion.containsPoint(x, y, z)) {
			writeVoxel(wrapper, glm::ivec3(x, y, z), air);
		}
	};
	voxelutil::visitVolume(_snapshotVolume, _snapshotRegion, funcErase, voxelutil::VisitSolid());

	if (_transformMode == TransformMode::Scale || _transformMode == TransformMode::Rotate) {
		voxel::RawVolume snapVol(_snapshotRegion);
		_snapshotVolume.copyTo(snapVol);

		const glm::vec3 dims(_snapshotRegion.getDimensionsInVoxels());
		voxel::RawVolume *result = nullptr;
		if (_transformMode == TransformMode::Scale) {
			// scaleVolume computes srcPivot = lower + normalizedPivot * dims
			const glm::vec3 lower(_snapshotRegion.getLowerCorner());
			const glm::vec3 normalizedPivot = (_snapshotCenter - lower) / dims;
			result = voxelutil::scaleVolume(&snapVol, _scale, normalizedPivot, _scaleSampling);
		} else {
			// rotateVolume computes pivot = normalizedPivot * dims (absolute offset)
			const glm::vec3 normalizedPivot = _snapshotCenter / dims;
			const glm::mat4 rotMat = glm::eulerAngleXYZ(
				glm::radians(_rotationDegrees.x),
				glm::radians(_rotationDegrees.y),
				glm::radians(_rotationDegrees.z));
			result = voxelutil::rotateVolume(&snapVol, rotMat, normalizedPivot);
		}
		if (result) {
			auto func = [&](int x, int y, int z, const voxel::Voxel &v) {
				writeVoxel(wrapper, glm::ivec3(x, y, z), v);
			};
			voxelutil::visitVolume(*result, func);
			delete result;
		}
	} else {
		// Forward mapping: Move and Shear produce 1:1 mappings without gaps
		// NOTE: we re-read the voxel from the sparse volume instead of using the visitor's
		// voxel parameter because SparseVolume::Sampler::voxel() returns a reference to an
		// internal member that becomes stale after the sampler advances.
		auto func = [&](int x, int y, int z, const voxel::Voxel &v) {
			const glm::ivec3 newPos = transformPosition(glm::ivec3(x, y, z));
			writeVoxel(wrapper, newPos, v);
		};
		voxelutil::visitVolume(_snapshotVolume, _snapshotRegion, func, voxelutil::VisitSolid());
	}

	// Prune interior voxels: remove any transformed voxel that is fully enclosed
	// by 6 solid neighbors. These are invisible and waste sparse storage.
	// Collect all positions first (read-only), then prune - mutating during
	// iteration would cause adjacent voxels to appear non-interior (checkerboard).
	auto isInterior = [&](const glm::ivec3 &pos) {
		for (int ni = 0; ni < lengthof(voxel::arrayPathfinderFaces); ++ni) {
			const glm::ivec3 nb = pos + voxel::arrayPathfinderFaces[ni];
			if (!volRegion.containsPoint(nb) || voxel::isAir(vol->voxel(nb).getMaterial())) {
				return false;
			}
		}
		return true;
	};

	const voxel::Region histRegion = _historyVolume.calculateRegion();
	core::DynamicArray<glm::ivec3> toPrune;
	if (histRegion.isValid()) {
		const glm::ivec3 &hLo = histRegion.getLowerCorner();
		const glm::ivec3 &hHi = histRegion.getUpperCorner();
		for (int z = hLo.z; z <= hHi.z; ++z) {
			for (int y = hLo.y; y <= hHi.y; ++y) {
				for (int x = hLo.x; x <= hHi.x; ++x) {
					if (!_historyVolume.hasVoxel(x, y, z)) {
						continue;
					}
					const glm::ivec3 pos(x, y, z);
					const voxel::Voxel &current = vol->voxel(pos);
					if (!voxel::isAir(current.getMaterial()) && isInterior(pos)) {
						toPrune.push_back(pos);
					}
				}
			}
		}
	}
	for (const glm::ivec3 &pos : toPrune) {
		vol->setVoxel(pos, air);
	}
	wrapper.addToDirtyRegion(toPrune);
}

void TransformBrush::generate(scenegraph::SceneGraph &, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
							  const voxel::Region &) {
	if (!_hasSnapshot) {
		return;
	}

	voxel::RawVolume *vol = wrapper.volume();

	// Restore previously transformed state before re-applying
	const voxel::Region histRegion = _historyVolume.calculateRegion();
	if (histRegion.isValid()) {
		const glm::ivec3 &hLo = histRegion.getLowerCorner();
		const glm::ivec3 &hHi = histRegion.getUpperCorner();
		for (int z = hLo.z; z <= hHi.z; ++z) {
			for (int y = hLo.y; y <= hHi.y; ++y) {
				for (int x = hLo.x; x <= hHi.x; ++x) {
					if (!_historyVolume.hasVoxel(x, y, z)) {
						continue;
					}
					if (vol->setVoxel(x, y, z, _historyVolume.voxel(x, y, z))) {
						wrapper.addToDirtyRegion(glm::ivec3(x, y, z));
					}
				}
			}
		}
	}
	_historyVolume.clear();

	// Apply the current transform from the original snapshot
	applyTransform(wrapper, ctx);
	markDirty();
}

bool TransformBrush::wantBrushGizmo(const BrushContext &ctx) const {
	return _hasSnapshot || ctx.targetVolumeRegion.isValid();
}

void TransformBrush::brushGizmoState(const BrushContext &ctx, BrushGizmoState &state) const {
	state.snap = (float)ctx.gridResolution;
	state.localMode = true;

	const glm::vec3 center = _hasSnapshot ? _snapshotCenter : glm::vec3(ctx.targetVolumeRegion.getCenter());

	switch (_transformMode) {
	case TransformMode::Move:
		state.operations = BrushGizmo_Translate;
		state.matrix = glm::translate(glm::mat4(1.0f), center + glm::vec3(_moveOffset));
		break;
	case TransformMode::Rotate: {
		state.operations = BrushGizmo_Rotate;
		const glm::vec3 radians = glm::radians(_rotationDegrees);
		const glm::mat4 rot = glm::eulerAngleYXZ(radians.y, radians.x, radians.z);
		state.matrix = glm::translate(glm::mat4(1.0f), center) * rot;
		state.snap = 15.0f; // 15 degree snap for rotation
		break;
	}
	case TransformMode::Scale:
		state.operations = BrushGizmo_Scale;
		state.matrix = glm::translate(glm::mat4(1.0f), center) * glm::scale(glm::mat4(1.0f), _scale);
		state.snap = 0.1f;
		break;
	case TransformMode::Shear:
		// Shear is not supported by vanilla ImGuizmo - will be handled by ImGuizmoEx later
		state.operations = BrushGizmo_None;
		break;
	default:
		state.operations = BrushGizmo_None;
		break;
	}
}

bool TransformBrush::applyBrushGizmo(BrushContext &ctx, const glm::mat4 &matrix,
									 const glm::mat4 &deltaMatrix, uint32_t operation) {
	const glm::vec3 center = _hasSnapshot ? _snapshotCenter : glm::vec3(ctx.targetVolumeRegion.getCenter());

	switch (_transformMode) {
	case TransformMode::Move: {
		// Extract absolute position from the gizmo matrix and compute offset from center
		const glm::vec3 newPos(matrix[3]);
		const glm::ivec3 offset = glm::ivec3(glm::round(newPos - center));
		if (offset != _moveOffset) {
			setMoveOffset(offset);
			markDirty();
			return true;
		}
		break;
	}
	case TransformMode::Rotate: {
		// Extract rotation from manipulated matrix (translation removed)
		glm::vec3 scale;
		glm::vec3 translation;
		glm::quat orientation;
		glm::vec3 skew;
		glm::vec4 perspective;
		glm::decompose(matrix, scale, orientation, translation, skew, perspective);
		const glm::vec3 euler = glm::degrees(glm::eulerAngles(orientation));
		if (euler != _rotationDegrees) {
			setRotationDegrees(euler);
			markDirty();
			return true;
		}
		break;
	}
	case TransformMode::Scale: {
		// Extract scale from manipulated matrix
		glm::vec3 newScale;
		glm::vec3 translation;
		glm::quat orientation;
		glm::vec3 skew;
		glm::vec4 perspective;
		glm::decompose(matrix, newScale, orientation, translation, skew, perspective);
		if (newScale != _scale) {
			setScale(newScale);
			markDirty();
			return true;
		}
		break;
	}
	default:
		break;
	}
	return false;
}

} // namespace voxedit
