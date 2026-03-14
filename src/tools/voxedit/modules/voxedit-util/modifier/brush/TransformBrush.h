/**
 * @file
 */

#pragma once

#include "Brush.h"
#include "core/GLM.h"
#include "voxel/SparseVolume.h"
#include "voxel/Voxel.h"

#include <glm/vec3.hpp>

namespace voxedit {

/**
 * @brief Transform mode for the TransformBrush
 */
enum class TransformMode : uint8_t {
	Move,
	Shear,
	Scale,
	Rotate,

	Max
};

/**
 * @brief Transforms selected voxels: move, shear, scale, rotate
 *
 * When the brush becomes active it captures the original selected voxels
 * in preExecute() from the real volume.
 * Consecutive parameter changes always re-transform from the original snapshot
 * so transforms are absolute, not incremental.
 * On finalize (reset/brush switch) the final state is committed to the undo stack.
 *
 * @ingroup Brushes
 */
class TransformBrush : public Brush {
private:
	using Super = Brush;

	TransformMode _transformMode = TransformMode::Move;
	voxel::VoxelSampling _voxelSampling = voxel::VoxelSampling::Nearest;
	bool _active = false;
	bool _hasSnapshot = false;
	voxel::RawVolume *_lastVolume = nullptr;

	// Original selected voxels captured at brush activation
	voxel::SparseVolume _snapshot;
	// Selection bounding box at capture time
	voxel::Region _snapshotRegion;
	// Center of selection (used as pivot for scale/rotate)
	glm::vec3 _snapshotCenter{0.0f};
	// Volume region lower corner at snapshot capture time (to detect region shifts)
	glm::ivec3 _capturedVolumeLower{0};

	// Per-generate bookkeeping: tracks positions written during a single generate()
	// call so the previous state can be restored before re-applying the transform.
	// Stores the original voxel at each modified position (with empty voxel storage enabled).
	voxel::SparseVolume _history;

	// Cached region for preview (union of snapshot + transformed bounding box)
	voxel::Region _cachedRegion;
	bool _cachedRegionValid = false;

	// Transform parameters
	glm::ivec3 _moveOffset{0};
	glm::ivec3 _shearOffset{0};
	glm::vec3 _scale{1.0f, 1.0f, 1.0f};
	glm::vec3 _rotationDegrees{0.0f, 0.0f, 0.0f};

	void captureSnapshot(const voxel::RawVolume *volume, const voxel::Region &volRegion);
	void adjustSnapshotForRegionShift(const glm::ivec3 &delta);
	void applyTransform(ModifierVolumeWrapper &wrapper, const BrushContext &ctx);
	void eraseSnapshotPositions(ModifierVolumeWrapper &wrapper);
	void applyInverseMapping(ModifierVolumeWrapper &wrapper);
	voxel::Region computeTransformedRegion() const;
	glm::ivec3 transformPosition(const glm::ivec3 &pos) const;
	glm::vec3 inverseTransformPosition(const glm::ivec3 &pos) const;
	void saveToHistory(voxel::RawVolume *vol, const glm::ivec3 &pos);
	void writeVoxel(ModifierVolumeWrapper &wrapper, const glm::ivec3 &pos, const voxel::Voxel &voxel);

protected:
	void generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
				  const voxel::Region &region) override;

public:
	static constexpr int MaxMoveOffset = 128;
	static constexpr int MaxShearOffset = 128;
	TransformBrush() : Super(BrushType::Transform, ModifierType::Override, ModifierType::Override) {
		_history.setStoreEmptyVoxels(true);
	}
	virtual ~TransformBrush() = default;

	void onSceneChange() override;
	void reset() override;
	void onActivated() override;
	bool onDeactivated() override;
	void preExecute(const BrushContext &ctx, const voxel::RawVolume *volume) override;
	bool beginBrush(const BrushContext &ctx) override;
	void endBrush(BrushContext &ctx) override;
	bool active() const override;
	voxel::Region calcRegion(const BrushContext &ctx) const override;
	bool managesOwnSelection() const override {
		return true;
	}

	TransformMode transformMode() const {
		return _transformMode;
	}

	void setTransformMode(TransformMode mode) {
		if (_transformMode != mode) {
			commitCurrentTransform();
			_transformMode = mode;
		}
	}

	/**
	 * @brief Commit the current transform: clear history (keeping the volume as-is)
	 * and re-capture the snapshot from the current volume state on next execute.
	 * Called when switching transform modes so the new mode starts from the
	 * result of the previous one rather than jumping back to the original.
	 */
	void commitCurrentTransform() {
		_history.clear();
		_snapshot.clear();
		_hasSnapshot = false;
		_cachedRegionValid = false;
		_cachedRegion = voxel::Region::InvalidRegion;
		_moveOffset = glm::ivec3(0);
		_shearOffset = glm::ivec3(0);
		_scale = glm::vec3(1.0f);
		_rotationDegrees = glm::vec3(0.0f);
	}

	voxel::VoxelSampling voxelSampling() const {
		return _voxelSampling;
	}

	void setVoxelSampling(voxel::VoxelSampling sampling) {
		_voxelSampling = sampling;
	}

	const glm::ivec3 &moveOffset() const {
		return _moveOffset;
	}

	void setMoveOffset(const glm::ivec3 &offset) {
		_moveOffset = glm::clamp(offset, glm::ivec3(-MaxMoveOffset), glm::ivec3(MaxMoveOffset));
	}

	const glm::ivec3 &shearOffset() const {
		return _shearOffset;
	}

	void setShearOffset(const glm::ivec3 &offset) {
		_shearOffset = glm::clamp(offset, glm::ivec3(-MaxShearOffset), glm::ivec3(MaxShearOffset));
	}

	const glm::vec3 &scale() const {
		return _scale;
	}

	void setScale(const glm::vec3 &scale) {
		_scale = glm::max(scale, glm::vec3(0.01f));
	}

	const glm::vec3 &rotationDegrees() const {
		return _rotationDegrees;
	}

	void setRotationDegrees(const glm::vec3 &degrees) {
		_rotationDegrees = degrees;
	}

	bool hasSnapshot() const {
		return _hasSnapshot;
	}

	size_t snapshotVoxelCount() const {
		return _snapshot.size();
	}

	bool wantBrushGizmo(const BrushContext &ctx) const override;
	void brushGizmoState(const BrushContext &ctx, BrushGizmoState &state) const override;
	bool applyBrushGizmo(BrushContext &ctx, const glm::mat4 &matrix,
						 const glm::mat4 &deltaMatrix, uint32_t operation) override;
};

} // namespace voxedit
