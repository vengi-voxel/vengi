/**
 * @file
 */

#pragma once

#include "SelectStrategy.h"
#include "voxel/Region.h"

namespace voxedit {
namespace select {

class Box3D : public Strategy {
private:
	voxel::Region _selectionRegion = voxel::Region::InvalidRegion;

public:
	bool isSimplePreview() const override { return true; }
	void generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
				  const voxel::Region &region, const AABBBrushState &state) override;
	bool needsAdditionalAction(const BrushContext &ctx) const override;
	void reset() override;

	const voxel::Region &selectionRegion() const {
		return _selectionRegion;
	}
	void setSelectionRegion(const voxel::Region &region) {
		_selectionRegion = region;
	}
};

} // namespace select
} // namespace voxedit
