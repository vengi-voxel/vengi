/**
 * @file
 */

#pragma once

#include "Brush.h"
#include "operation/TransformOperation.h"
#include "app/I18N.h"
#include "core/ArrayLength.h"
#include "core/GLM.h"
#include "ui/IconsLucide.h"
#include "voxel/Voxel.h"

#include <glm/vec3.hpp>

namespace voxedit {

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
 * Changes stay in a draft until Apply (or a tool change with a valid preview).
 * Cancel or Escape discards the draft.
 *
 * @ingroup Brushes
 */
class TransformBrush : public Brush, public TransformOperation {
private:
	using Super = Brush;

	bool _active = false;

protected:
	void generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
				  const voxel::Region &region) override;

public:
	static constexpr int MaxMoveOffset = 250;
	static constexpr int MaxShearOffset = 250;

	TransformBrush() : Super(BrushType::Transform, ModifierType::Override, ModifierType::Override) {
	}
	TransformBrush(const TransformBrush &other);
	virtual ~TransformBrush() = default;

	void onSceneChange() override;
	void reset() override;
	void onActivated() override;
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
	 * @brief Commit the current transform: keep the volume as-is and recapture
	 * the snapshot from the current volume state on the next evaluate.
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

	void handles(const BrushContext &ctx, core::DynamicArray<BrushGizmoState> &output) const override;
	bool hasHandle(const BrushContext &ctx) const;
	void describeHandle(const BrushContext &ctx, BrushGizmoState &state) const;
	bool editHandle(uint32_t handleId, BrushContext &ctx, const glm::mat4 &matrix, const glm::mat4 &deltaMatrix,
						 uint32_t operation) override;
};

inline bool TransformBrush::managesOwnSelection() const {
	return true;
}

inline TransformMode TransformBrush::transformMode() const {
	return _parameters.mode;
}

inline voxel::VoxelSampling TransformBrush::voxelSampling() const {
	return _parameters.sampling;
}

inline void TransformBrush::setVoxelSampling(voxel::VoxelSampling sampling) {
	markSettingsChanged();
	_parameters.sampling = sampling;
}

inline const glm::ivec3 &TransformBrush::moveOffset() const {
	return _parameters.translation;
}

inline void TransformBrush::setMoveOffset(const glm::ivec3 &offset) {
	markSettingsChanged();
	_parameters.translation = glm::clamp(offset, glm::ivec3(-MaxMoveOffset), glm::ivec3(MaxMoveOffset));
}

inline const glm::ivec3 &TransformBrush::shearOffset() const {
	return _parameters.shear;
}

inline void TransformBrush::setShearOffset(const glm::ivec3 &offset) {
	markSettingsChanged();
	_parameters.shear = glm::clamp(offset, glm::ivec3(-MaxShearOffset), glm::ivec3(MaxShearOffset));
}

inline const glm::vec3 &TransformBrush::scale() const {
	return _parameters.scale;
}

inline void TransformBrush::setScale(const glm::vec3 &scale) {
	markSettingsChanged();
	_parameters.scale = glm::max(scale, glm::vec3(0.01f));
}

inline const glm::vec3 &TransformBrush::rotationDegrees() const {
	return _parameters.rotationDegrees;
}

inline void TransformBrush::setRotationDegrees(const glm::vec3 &degrees) {
	markSettingsChanged();
	_parameters.rotationDegrees = degrees;
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
