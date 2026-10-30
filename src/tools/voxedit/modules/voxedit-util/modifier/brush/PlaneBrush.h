/**
 * @file
 */

#pragma once

#include "AABBBrush.h"

namespace voxedit {

/**
 * @brief A brush that generates voxels on a whole plane or extrudes on existing voxels
 * @ingroup Brushes
 */
class PlaneBrush : public AABBBrush {
private:
	using Super = AABBBrush;
	voxel::Voxel _hitVoxel;
	glm::ivec3 _initialPlanePos{0};

protected:
	void generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
				  const voxel::Region &region) override;
	int calculateThickness(const BrushContext &ctx) const;

public:
	PlaneBrush() : Super(BrushType::Plane) {
	}
	bool previewNeedsExistingVolume() const override {
		return true;
	}
	int previewPadding() const override {
		return 1;
	}
	BrushPreviewStyle previewStyle() const override {
		return BrushPreviewStyle::Changes;
	}
	voxel::Region previewRegion(const BrushContext &ctx, const voxel::RawVolume *volume) const override;
	bool projectCursor(const math::Ray &ray, glm::ivec3 &position) const override;
	bool needsAdditionalAction(const BrushContext &) const override {
		return false;
	}
	void handles(const BrushContext &, core::DynamicArray<BrushGizmoState> &) const override {
	}
	virtual ~PlaneBrush() = default;

	void reset() override;
	bool beginBrush(const BrushContext &ctx) override;
	void preExecute(const BrushContext &ctx, const voxel::RawVolume *volume) override;
};

} // namespace voxedit
