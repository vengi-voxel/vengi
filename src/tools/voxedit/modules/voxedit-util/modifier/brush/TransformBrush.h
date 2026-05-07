/**
 * @file
 */

#pragma once

#include "Brush.h"
#include "SnapshotHelper.h"
#include "app/I18N.h"
#include "core/ArrayLength.h"
#include "core/GLM.h"
#include "ui/IconsLucide.h"
#include "voxel/Voxel.h"

#include <glm/vec3.hpp>

namespace voxedit {

enum class TransformMode : uint8_t {
	Move,
	Shear,
	Scale,
	Rotate,

	Max
};

// clang-format off
static constexpr const char *TransformModeStr[] = {
	NC_("Transform Modes", "Move"),  NC_("Transform Modes", "Shear"),
	NC_("Transform Modes", "Scale"), NC_("Transform Modes", "Rotate")};
static_assert(lengthof(TransformModeStr) == (int)TransformMode::Max, "TransformModeStr size mismatch");

static constexpr const char *TransformModeIcons[] = {
	ICON_LC_MOVE, ICON_LC_ITALIC, ICON_LC_SCALING, ICON_LC_ROTATE_3D};
static_assert(lengthof(TransformModeIcons) == (int)TransformMode::Max, "TransformModeIcons size mismatch");
// clang-format on

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
	voxel::RawVolume *_lastVolume = nullptr;

	SnapshotHelper _snapshotHelper;
	// Center of selection (used as pivot for scale/rotate)
	glm::vec3 _snapshotCenter{0.0f};

	// Cached region for preview (union of snapshot + transformed bounding box)
	voxel::Region _cachedRegion;
	bool _cachedRegionValid = false;
	// Previous frame's transformed destination region (for dirty region tracking)
	mutable voxel::Region _prevTransformedRegion = voxel::Region::InvalidRegion;

	// Transform parameters
	glm::ivec3 _moveOffset{0};
	glm::ivec3 _shearOffset{0};
	glm::vec3 _scale{1.0f, 1.0f, 1.0f};
	glm::vec3 _rotationDegrees{0.0f, 0.0f, 0.0f};

	void applyTransform(ModifierVolumeWrapper &wrapper, const BrushContext &ctx);
	void eraseSnapshotPositions(ModifierVolumeWrapper &wrapper);
	void applyInverseMapping(ModifierVolumeWrapper &wrapper);
	voxel::Region computeTransformedRegion() const;
	voxel::Region computeDestinationRegion() const;
	glm::ivec3 transformPosition(const glm::ivec3 &pos) const;
	glm::vec3 inverseTransformPosition(const glm::ivec3 &pos) const;

protected:
	void generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
				  const voxel::Region &region) override;

public:
	static constexpr int MaxMoveOffset = 250;
	static constexpr int MaxShearOffset = 250;

	TransformBrush() : Super(BrushType::Transform, ModifierType::Override, ModifierType::Override) {
	}
	virtual ~TransformBrush() = default;

	void onSceneChange() override;
	void reset() override;
	void onActivated() override;
	bool hasPendingChanges() const override;
	bool onDeactivated() override;
	voxel::Region revertChanges(voxel::RawVolume *volume) override;
	void preExecute(const BrushContext &ctx, const voxel::RawVolume *volume) override;
	bool beginBrush(const BrushContext &ctx) override;
	void endBrush(BrushContext &ctx) override;
	bool active() const override;
	voxel::Region calcRegion(const BrushContext &ctx) const override;
	bool dirtyRegions(core::Buffer<voxel::Region> &regions) const override;
	bool managesOwnSelection() const override;

	TransformMode transformMode() const;

	void setTransformMode(TransformMode mode);

	/**
	 * @brief Commit the current transform: clear history (keeping the volume as-is)
	 * and re-capture the snapshot from the current volume state on next execute.
	 * Called when switching transform modes so the new mode starts from the
	 * result of the previous one rather than jumping back to the original.
	 */
	void commitCurrentTransform();

	voxel::VoxelSampling voxelSampling() const;
	void setVoxelSampling(voxel::VoxelSampling sampling);

	const glm::ivec3 &moveOffset() const;
	void setMoveOffset(const glm::ivec3 &offset);

	const glm::ivec3 &shearOffset() const;
	void setShearOffset(const glm::ivec3 &offset);

	const glm::vec3 &scale() const;
	void setScale(const glm::vec3 &scale);

	const glm::vec3 &rotationDegrees() const;
	void setRotationDegrees(const glm::vec3 &degrees);

	bool hasSnapshot() const;
	size_t snapshotVoxelCount() const;
	const voxel::Region &snapshotRegion() const;

	bool wantBrushGizmo(const BrushContext &ctx) const override;
	void brushGizmoState(const BrushContext &ctx, BrushGizmoState &state) const override;
	bool applyBrushGizmo(BrushContext &ctx, const glm::mat4 &matrix, const glm::mat4 &deltaMatrix,
						 uint32_t operation) override;
};

inline bool TransformBrush::managesOwnSelection() const {
	return true;
}

inline TransformMode TransformBrush::transformMode() const {
	return _transformMode;
}

inline voxel::VoxelSampling TransformBrush::voxelSampling() const {
	return _voxelSampling;
}

inline void TransformBrush::setVoxelSampling(voxel::VoxelSampling sampling) {
	_voxelSampling = sampling;
}

inline const glm::ivec3 &TransformBrush::moveOffset() const {
	return _moveOffset;
}

inline void TransformBrush::setMoveOffset(const glm::ivec3 &offset) {
	_moveOffset = glm::clamp(offset, glm::ivec3(-MaxMoveOffset), glm::ivec3(MaxMoveOffset));
}

inline const glm::ivec3 &TransformBrush::shearOffset() const {
	return _shearOffset;
}

inline void TransformBrush::setShearOffset(const glm::ivec3 &offset) {
	_shearOffset = glm::clamp(offset, glm::ivec3(-MaxShearOffset), glm::ivec3(MaxShearOffset));
}

inline const glm::vec3 &TransformBrush::scale() const {
	return _scale;
}

inline void TransformBrush::setScale(const glm::vec3 &scale) {
	_scale = glm::max(scale, glm::vec3(0.01f));
}

inline const glm::vec3 &TransformBrush::rotationDegrees() const {
	return _rotationDegrees;
}

inline void TransformBrush::setRotationDegrees(const glm::vec3 &degrees) {
	_rotationDegrees = degrees;
}

inline bool TransformBrush::hasSnapshot() const {
	return _snapshotHelper.hasSnapshot();
}

inline size_t TransformBrush::snapshotVoxelCount() const {
	return _snapshotHelper.snapshotVoxelCount();
}

inline const voxel::Region &TransformBrush::snapshotRegion() const {
	return _snapshotHelper.snapshotRegion();
}

} // namespace voxedit
