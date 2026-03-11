/**
 * @file
 */

#include "TransformBrush.h"
#include "core/collection/DynamicSet.h"
#include "math/Axis.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/Connectivity.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/trigonometric.hpp>
#include <glm/common.hpp>

namespace voxedit {

using PositionSet = core::DynamicSet<glm::ivec3, 1031, glm::hash<glm::ivec3>>;

void TransformBrush::onSceneChange() {
	Super::onSceneChange();
	_active = false;
	_hasSnapshot = false;
	_snapshot.clear();
	_history.clear();
	_historyPositions.clear();
	_snapshotRegion = voxel::Region::InvalidRegion;
	_cachedRegion = voxel::Region::InvalidRegion;
	_cachedRegionValid = false;
	_moveOffset = glm::ivec3(0);
	_shearOffset = glm::ivec3(0);
	_scale = glm::vec3(1.0f);
	_rotationDegrees = glm::vec3(0.0f);
}

void TransformBrush::onActivated() {
	reset();
}

bool TransformBrush::onDeactivated() {
	return _hasSnapshot;
}

void TransformBrush::reset() {
	Super::reset();
	_active = false;
	_hasSnapshot = false;
	_snapshot.clear();
	_history.clear();
	_historyPositions.clear();
	_snapshotRegion = voxel::Region::InvalidRegion;
	_cachedRegion = voxel::Region::InvalidRegion;
	_cachedRegionValid = false;
	_snapshotCenter = glm::vec3(0.0f);
	_capturedVolumeLower = glm::ivec3(0);
	_moveOffset = glm::ivec3(0);
	_shearOffset = glm::ivec3(0);
	_scale = glm::vec3(1.0f);
	_rotationDegrees = glm::vec3(0.0f);
	_transformMode = TransformMode::Move;
	_scaleSampling = voxelutil::ScaleSampling::Nearest;
	_lastVolume = nullptr;
}

bool TransformBrush::beginBrush(const BrushContext &ctx) {
	if (_active) {
		return false;
	}
	_active = true;
	return true;
}

void TransformBrush::endBrush(BrushContext &) {
	// Prune interior voxels on final commit. Only for Scale/Rotate which can
	// create new interior voxels. Move/Shear are 1:1 mappings that preserve topology.
	const bool useInverseMapping = (_transformMode == TransformMode::Scale ||
									_transformMode == TransformMode::Rotate);
	if (_lastVolume && useInverseMapping) {
		const voxel::Region &volRegion = _lastVolume->region();
		const voxel::Voxel air;
		auto isInterior = [&](const glm::ivec3 &pos) {
			for (int ni = 0; ni < lengthof(voxel::arrayPathfinderFaces); ++ni) {
				const glm::ivec3 nb = pos + voxel::arrayPathfinderFaces[ni];
				if (!volRegion.containsPoint(nb) || voxel::isAir(_lastVolume->voxel(nb).getMaterial())) {
					return false;
				}
			}
			return true;
		};

		core::DynamicArray<glm::ivec3> toPrune;
		toPrune.reserve(_history.size());
		for (const HistoryEntry &entry : _history) {
			const glm::ivec3 &pos = entry.pos;
			if (!voxel::isAir(_lastVolume->voxel(pos).getMaterial()) && isInterior(pos)) {
				toPrune.push_back(pos);
			}
		}
		for (const glm::ivec3 &pos : toPrune) {
			_lastVolume->setVoxel(pos, air);
		}
	}
	_lastVolume = nullptr;
	_active = false;
}

bool TransformBrush::active() const {
	return _active || _hasSnapshot;
}

void TransformBrush::preExecute(const BrushContext &ctx, const voxel::RawVolume *volume) {
	if (!_hasSnapshot && volume != nullptr) {
		captureSnapshot(volume, ctx.targetVolumeRegion);
	} else if (_hasSnapshot) {
		// Detect volume region shift (e.g. node was moved) and adjust cached data
		const glm::ivec3 delta = ctx.targetVolumeRegion.getLowerCorner() - _capturedVolumeLower;
		if (delta != glm::ivec3(0)) {
			adjustSnapshotForRegionShift(delta);
		}
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
	_snapshotCenter = glm::vec3(selLo + selHi) * 0.5f;
	_capturedVolumeLower = volRegion.getLowerCorner();
	_hasSnapshot = true;
}

void TransformBrush::adjustSnapshotForRegionShift(const glm::ivec3 &delta) {
	// Collect and re-insert with shifted positions
	struct ShiftEntry {
		glm::ivec3 pos;
		voxel::Voxel voxel;
	};
	core::DynamicArray<ShiftEntry> entries;
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
	_snapshotCenter += glm::vec3(delta);
	_capturedVolumeLower += delta;

	// Shift history positions in-place
	_historyPositions.clear();
	for (HistoryEntry &entry : _history) {
		entry.pos += delta;
		_historyPositions.insert(entry.pos);
	}
}

voxel::Region TransformBrush::computeTransformedRegion() const {
	if (!_hasSnapshot || _snapshot.empty()) {
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

glm::vec3 TransformBrush::inverseTransformPosition(const glm::ivec3 &pos) const {
	const glm::vec3 center = _snapshotCenter;
	glm::vec3 result(pos);

	switch (_transformMode) {
	case TransformMode::Scale: {
		// Inverse of: result = center + relative * scale
		const glm::vec3 relative = result - center;
		result = center + relative / _scale;
		break;
	}

	case TransformMode::Rotate: {
		// Inverse rotation: apply in reverse order with negated angles
		glm::vec3 relative = result - center;
		if (_rotationDegrees.z != 0.0f) {
			relative = glm::rotateZ(relative, glm::radians(-_rotationDegrees.z));
		}
		if (_rotationDegrees.y != 0.0f) {
			relative = glm::rotateY(relative, glm::radians(-_rotationDegrees.y));
		}
		if (_rotationDegrees.x != 0.0f) {
			relative = glm::rotateX(relative, glm::radians(-_rotationDegrees.x));
		}
		result = center + relative;
		break;
	}

	default:
		break;
	}

	return result;
}

void TransformBrush::saveToHistory(voxel::RawVolume *vol, const glm::ivec3 &pos) {
	if (_historyPositions.has(pos)) {
		return;
	}
	_historyPositions.insert(pos);
	_history.push_back({pos, vol->voxel(pos)});
}

void TransformBrush::writeVoxel(voxel::RawVolume *vol, ModifierVolumeWrapper &wrapper,
								const glm::ivec3 &pos, const voxel::Voxel &newVoxel) {
	if (!vol->region().containsPoint(pos)) {
		return;
	}
	saveToHistory(vol, pos);
	if (vol->setVoxel(pos, newVoxel)) {
		wrapper.addToDirtyRegion(pos);
	}
}

const voxel::Voxel *TransformBrush::findNearestSnapshotVoxel(const glm::vec3 &srcPos) const {
	// Half-diagonal of a unit cube - maximum distance for nearest-neighbor sampling
	static constexpr float MaxSampleDistance = 0.87f;

	const glm::ivec3 rounded = glm::ivec3(glm::round(srcPos));
	// Fast path: exact match via hash lookup
	if (_snapshot.hasVoxel(rounded)) {
		return &_snapshot.voxel(rounded);
	}

	// Check immediate neighbors (3x3x3 cube around the rounded position)
	const voxel::Voxel *best = nullptr;
	float bestDist = MaxSampleDistance + 1.0f;
	for (int dz = -1; dz <= 1; ++dz) {
		for (int dy = -1; dy <= 1; ++dy) {
			for (int dx = -1; dx <= 1; ++dx) {
				const glm::ivec3 neighbor(rounded.x + dx, rounded.y + dy, rounded.z + dz);
				if (!_snapshot.hasVoxel(neighbor)) {
					continue;
				}
				const float dist = glm::length(glm::vec3(neighbor) - srcPos);
				if (dist < bestDist) {
					bestDist = dist;
					best = &_snapshot.voxel(neighbor);
				}
			}
		}
	}

	if (best && bestDist <= MaxSampleDistance) {
		return best;
	}
	return nullptr;
}

const voxel::Voxel *TransformBrush::findLinearSnapshotVoxel(const glm::vec3 &srcPos) const {
	// Trilinear-style sampling for voxels: check the 2x2x2 cell containing srcPos.
	// Since voxel materials are discrete, we pick the most common non-air material
	// (majority vote) among the 8 corners of the cell.
	const glm::ivec3 base = glm::ivec3(glm::floor(srcPos));
	static constexpr int CellSize = 2;
	static constexpr int CellCorners = 8;

	// Count occurrences of each material in the 2x2x2 cell
	struct MaterialCount {
		const voxel::Voxel *voxel;
		int count;
	};
	static constexpr int MaxMaterials = CellCorners;
	MaterialCount materials[MaxMaterials] = {};
	int materialCount = 0;

	for (int dz = 0; dz < CellSize; ++dz) {
		for (int dy = 0; dy < CellSize; ++dy) {
			for (int dx = 0; dx < CellSize; ++dx) {
				const glm::ivec3 pos(base.x + dx, base.y + dy, base.z + dz);
				if (!_snapshot.hasVoxel(pos)) {
					continue;
				}
				const voxel::Voxel *v = &_snapshot.voxel(pos);
				const uint8_t color = v->getColor();
				bool found = false;
				for (int mi = 0; mi < materialCount; ++mi) {
					if (materials[mi].voxel->getColor() == color) {
						materials[mi].count++;
						found = true;
						break;
					}
				}
				if (!found) {
					materials[materialCount++] = {v, 1};
				}
			}
		}
	}

	if (materialCount == 0) {
		return nullptr;
	}

	// Return the material with the highest count
	int bestIdx = 0;
	for (int mi = 1; mi < materialCount; ++mi) {
		if (materials[mi].count > materials[bestIdx].count) {
			bestIdx = mi;
		}
	}
	return materials[bestIdx].voxel;
}

const voxel::Voxel *TransformBrush::findCubicSnapshotVoxel(const glm::vec3 &srcPos) const {
	// Cubic sampling: check a 4x4x4 neighborhood centered on srcPos.
	// Each source voxel votes with a weight inversely proportional to its distance.
	// The material with the highest total weight wins.
	static constexpr int HalfExtent = 2;
	static constexpr float MaxCubicDistance = 3.5f;

	const glm::ivec3 center = glm::ivec3(glm::round(srcPos));

	struct MaterialWeight {
		const voxel::Voxel *voxel;
		float weight;
	};
	static constexpr int MaxMaterials = 64; // 4x4x4 worst case
	MaterialWeight materials[MaxMaterials] = {};
	int materialCount = 0;

	for (int dz = -HalfExtent + 1; dz <= HalfExtent; ++dz) {
		for (int dy = -HalfExtent + 1; dy <= HalfExtent; ++dy) {
			for (int dx = -HalfExtent + 1; dx <= HalfExtent; ++dx) {
				const glm::ivec3 pos(center.x + dx, center.y + dy, center.z + dz);
				if (!_snapshot.hasVoxel(pos)) {
					continue;
				}
				const float dist = glm::length(glm::vec3(pos) - srcPos);
				if (dist > MaxCubicDistance) {
					continue;
				}
				// Weight: inverse distance (closer voxels contribute more)
				const float weight = 1.0f / (dist + 0.001f);
				const voxel::Voxel *v = &_snapshot.voxel(pos);
				const uint8_t color = v->getColor();
				bool found = false;
				for (int mi = 0; mi < materialCount; ++mi) {
					if (materials[mi].voxel->getColor() == color) {
						materials[mi].weight += weight;
						found = true;
						break;
					}
				}
				if (!found && materialCount < MaxMaterials) {
					materials[materialCount++] = {v, weight};
				}
			}
		}
	}

	if (materialCount == 0) {
		return nullptr;
	}

	// Return the material with the highest accumulated weight
	int bestIdx = 0;
	for (int mi = 1; mi < materialCount; ++mi) {
		if (materials[mi].weight > materials[bestIdx].weight) {
			bestIdx = mi;
		}
	}
	return materials[bestIdx].voxel;
}

const voxel::Voxel *TransformBrush::sampleSnapshotVoxel(const glm::vec3 &srcPos) const {
	switch (_scaleSampling) {
	case voxelutil::ScaleSampling::Linear:
		return findLinearSnapshotVoxel(srcPos);
	case voxelutil::ScaleSampling::Cubic:
		return findCubicSnapshotVoxel(srcPos);
	case voxelutil::ScaleSampling::Nearest:
	default:
		return findNearestSnapshotVoxel(srcPos);
	}
}

void TransformBrush::applyTransform(ModifierVolumeWrapper &wrapper, const BrushContext &ctx) {
	voxel::RawVolume *vol = wrapper.volume();
	const voxel::Region &volRegion = vol->region();
	const voxel::Voxel air;

	// Erase all original selected voxels (saving to history)
	{
		const glm::ivec3 &lo = _snapshotRegion.getLowerCorner();
		const glm::ivec3 &hi = _snapshotRegion.getUpperCorner();
		for (int z = lo.z; z <= hi.z; ++z) {
			for (int y = lo.y; y <= hi.y; ++y) {
				for (int x = lo.x; x <= hi.x; ++x) {
					if (!_snapshot.hasVoxel(x, y, z)) {
						continue;
					}
					const glm::ivec3 pos(x, y, z);
					if (volRegion.containsPoint(pos)) {
						writeVoxel(vol, wrapper, pos, air);
					}
				}
			}
		}
	}

	const bool useInverseMapping = (_transformMode == TransformMode::Scale ||
									_transformMode == TransformMode::Rotate);

	// TODO: use VolumeRotator.h and VolumeRescaler.h code here
	if (useInverseMapping) {
		// Inverse mapping: compute the destination bounding box by transforming all
		// 8 corners of the snapshot region, then iterate every destination position
		// and inverse-map back into the source to find the voxel that fills it.
		const glm::ivec3 &srcLo = _snapshotRegion.getLowerCorner();
		const glm::ivec3 &srcHi = _snapshotRegion.getUpperCorner();
		glm::ivec3 dstLo(INT_MAX);
		glm::ivec3 dstHi(INT_MIN);

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

		// Clamp to volume bounds
		dstLo = glm::max(dstLo, volRegion.getLowerCorner());
		dstHi = glm::min(dstHi, volRegion.getUpperCorner());

		for (int dz = dstLo.z; dz <= dstHi.z; ++dz) {
			for (int dy = dstLo.y; dy <= dstHi.y; ++dy) {
				for (int dx = dstLo.x; dx <= dstHi.x; ++dx) {
					const glm::ivec3 dstPos(dx, dy, dz);
					const glm::vec3 srcPos = inverseTransformPosition(dstPos);
					const voxel::Voxel *source = sampleSnapshotVoxel(srcPos);
					if (source) {
						writeVoxel(vol, wrapper, dstPos, *source);
					}
				}
			}
		}
	} else {
		// Forward mapping: Move and Shear produce 1:1 mappings without gaps
		const glm::ivec3 &lo = _snapshotRegion.getLowerCorner();
		const glm::ivec3 &hi = _snapshotRegion.getUpperCorner();
		for (int z = lo.z; z <= hi.z; ++z) {
			for (int y = lo.y; y <= hi.y; ++y) {
				for (int x = lo.x; x <= hi.x; ++x) {
					if (!_snapshot.hasVoxel(x, y, z)) {
						continue;
					}
					const glm::ivec3 newPos = transformPosition(glm::ivec3(x, y, z));
					writeVoxel(vol, wrapper, newPos, _snapshot.voxel(x, y, z));
				}
			}
		}
	}

}

void TransformBrush::generate(scenegraph::SceneGraph &, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
							  const voxel::Region &) {
	if (!_hasSnapshot) {
		return;
	}

	voxel::RawVolume *vol = wrapper.volume();
	_lastVolume = vol;

	// Restore previously transformed state before re-applying
	for (const HistoryEntry &entry : _history) {
		if (vol->setVoxel(entry.pos, entry.original)) {
			wrapper.addToDirtyRegion(entry.pos);
		}
	}
	_history.clear();
	_historyPositions.clear();

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
		state.operations = BrushGizmo_Translate;
		state.matrix = glm::translate(glm::mat4(1.0f), center + glm::vec3(_shearOffset));
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
	case TransformMode::Shear: {
		// Extract absolute position from the gizmo matrix and compute offset from center
		const glm::vec3 newPos(matrix[3]);
		const glm::ivec3 offset = glm::ivec3(glm::round(newPos - center));
		if (offset != _shearOffset) {
			setShearOffset(offset);
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
