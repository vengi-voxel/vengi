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
	bool _accumulating = false;
	bool _growRegion = false;
	bool _hadSelection = false;
	voxel::Region _dirtyRegion = voxel::Region::InvalidRegion;
	voxel::Region _finalUndoRegion = voxel::Region::InvalidRegion;

public:
	void generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
				  const voxel::Region &region, const AABBBrushState &state) override;
	bool beginBrush(const BrushContext &ctx, const AABBBrushState &state) override;
	void endBrush(BrushContext &ctx) override;
	void abort(BrushContext &ctx) override;
	void reset() override;

	bool growRegion() const {
		return _growRegion;
	}
	void setGrowRegion(bool v) {
		_growRegion = v;
	}

	bool hasPendingChanges() const {
		return _accumulating && _dirtyRegion.isValid();
	}
	voxel::Region consumeFinalUndoRegion();
};

} // namespace select
} // namespace voxedit
