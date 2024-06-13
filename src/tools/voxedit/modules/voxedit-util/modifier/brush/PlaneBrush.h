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
	glm::ivec3 _initialPlanePos;

protected:
	void generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &context,
				  const voxel::Region &region) override;
	int calculateThickness(const BrushContext &context) const;
public:
	PlaneBrush() : Super(BrushType::Plane) {
	}
	virtual ~PlaneBrush() = default;

	void reset() override;
	bool start(const BrushContext &context) override;
	void preExecute(const BrushContext &ctx, const voxel::RawVolume *volume) override;
};

} // namespace voxedit
