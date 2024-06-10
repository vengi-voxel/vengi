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
	voxel::Region _region;

protected:
	void generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &context,
				  const voxel::Region &region) override;
	int calculateThickness(const BrushContext &context) const;
public:
	PlaneBrush() : Super(BrushType::Plane) {
	}
	virtual ~PlaneBrush() = default;

	bool start(const BrushContext &context) override;
	void preExecute(const BrushContext &ctx, const voxel::RawVolume *volume) override;
	voxel::Region calcRegion(const BrushContext &context) const override;
};

} // namespace voxedit
