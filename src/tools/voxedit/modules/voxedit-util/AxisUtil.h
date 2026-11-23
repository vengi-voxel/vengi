/**
 * @file
 */

#pragma once

#include "math/Axis.h"
#include "voxel/Region.h"
#include "video/ShapeBuilder.h"
#include "math/Vec4f.h"
#include "math/Vec3i.h"

namespace voxedit {

inline constexpr int getIndexForMirrorAxis(math::Axis axis) {
	if (axis == math::Axis::X) {
		return 2;
	} else if (axis == math::Axis::Y) {
		return 1;
	}
	return 0;
}

extern void updateShapeBuilderForPlane(video::ShapeBuilder& shapeBuilder, const voxel::Region& region, bool mirror, const math::Vec3i& pos, math::Axis axis, const math::Vec4f& color);

}
