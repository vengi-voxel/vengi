/**
 * @file
 */

#pragma once

#include "SelectStrategy.h"

namespace voxedit {
namespace select {

class All : public Strategy {
private:
	using Super = Strategy;

public:
	bool isSimplePreview() const override {
		return true;
	}
	voxel::Region calcRegion(const BrushContext &ctx, const AABBBrushState &state) const override {
		return voxel::Region::InvalidRegion;
	}
	void generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
				  const voxel::Region &region, const AABBBrushState &state) override;
	bool needsAdditionalAction(const BrushContext &ctx) const override;
};

} // namespace select
} // namespace voxedit
