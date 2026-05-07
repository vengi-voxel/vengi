/**
 * @file
 */

#pragma once

#include "SelectStrategy.h"
#include "color/Distance.h"

namespace voxedit {
namespace select {

class FuzzyColor : public Strategy {
private:
	using Super = Strategy;
	float _colorThreshold = color::ApproximationDistanceModerate;

public:
	void generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
				  const voxel::Region &region, const AABBBrushState &state) override;

	float colorThreshold() const {
		return _colorThreshold;
	}
	void setColorThreshold(float threshold) {
		_colorThreshold = threshold;
	}
};

} // namespace select
} // namespace voxedit
