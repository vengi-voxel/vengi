/**
 * @file
 */

#pragma once

#include "SelectStrategy.h"

namespace voxedit {
namespace select {

class All : public Strategy {
public:
	void generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
				  const voxel::Region &region, const AABBBrushState &state) override;
	bool needsAdditionalAction(const BrushContext &ctx) const override;
};

} // namespace select
} // namespace voxedit
