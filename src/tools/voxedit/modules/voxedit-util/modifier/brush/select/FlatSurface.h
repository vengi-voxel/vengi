/**
 * @file
 */

#pragma once

#include "SelectStrategy.h"
#include <glm/common.hpp>

namespace voxedit {
namespace select {

class FlatSurface : public Strategy {
private:
	int _flatDeviation = 0;

public:
	static constexpr int MaxDeviation = 32;

	void generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
				  const voxel::Region &region, const AABBBrushState &state) override;

	int deviation() const {
		return _flatDeviation;
	}
	void setDeviation(int d) {
		_flatDeviation = glm::clamp(d, 0, MaxDeviation);
	}
};

} // namespace select
} // namespace voxedit
