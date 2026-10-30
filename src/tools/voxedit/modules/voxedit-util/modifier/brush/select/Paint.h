/**
 * @file
 */

#pragma once

#include "SelectStrategy.h"
#include "voxel/Region.h"

namespace voxedit {
namespace select {

class Paint : public Strategy {
private:
	using Super = Strategy;
	bool _growRegion = false;

public:
	voxel::Region calcRegion(const BrushContext &ctx, const AABBBrushState &state) const override;
	void generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
				  const voxel::Region &region, const AABBBrushState &state) override;
	bool growRegion() const {
		return _growRegion;
	}
	void setGrowRegion(bool v) {
		_growRegion = v;
	}
};

} // namespace select
} // namespace voxedit
