/**
 * @file
 */

#pragma once

#include "SelectStrategy.h"

namespace voxedit {
namespace select {

class Surface : public Strategy {
public:
	bool isSimplePreview() const override { return true; }
	void generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
				  const voxel::Region &region, const AABBBrushState &state) override;
};

} // namespace select
} // namespace voxedit
