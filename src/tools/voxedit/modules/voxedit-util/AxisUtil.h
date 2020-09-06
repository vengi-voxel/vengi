/**
 * @file
 */

#pragma once

#include "math/Axis.h"
#include "voxel/Region.h"
#include "video/ShapeBuilder.h"
#include "core/GLM.h"

namespace voxedit {

inline constexpr int getIndexForAxis(math::Axis axis) {
	if (axis == math::Axis::X) {
		return 0;
	} else if (axis == math::Axis::Y) {
		return 1;
	}
	return 2;
}

inline constexpr int getIndexForMirrorAxis(math::Axis axis) {
	if (axis == math::Axis::X) {
		return 2;
	} else if (axis == math::Axis::Y) {
		return 1;
	}
	return 0;
}

extern void updateShapeBuilderForPlane(video::ShapeBuilder& shapeBuilder, const voxel::Region& region, bool mirror, const glm::ivec3& pos, math::Axis axis, const glm::vec4& color);

}
