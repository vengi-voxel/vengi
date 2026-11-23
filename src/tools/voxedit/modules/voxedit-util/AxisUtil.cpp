/**
 * @file
 */

#include "AxisUtil.h"
#include "core/ArrayLength.h"
#include "math/Vec4f.h"
#include "math/Vec3i.h"
#include "math/Vec3f.h"

namespace voxedit {

void updateShapeBuilderForPlane(video::ShapeBuilder& shapeBuilder, const voxel::Region& region, bool mirror, const math::Vec3i& pos, math::Axis axis, const math::Vec4f& color) {
	const int index = mirror ? getIndexForMirrorAxis(axis) : math::getIndexForAxis(axis);
	math::Vec3f mins = region.getLowerCorner();
	math::Vec3f maxs = region.getUpperCorner() + 1;
	mins[index] = maxs[index] = pos[index];
	const math::Vec3f& ll = mins;
	const math::Vec3f& ur = maxs;
	math::Vec3f ul;
	math::Vec3f lr;
	if (axis == math::Axis::Y) {
		ul = math::Vec3f(mins.x, mins.y, maxs.z);
		lr = math::Vec3f(maxs.x, maxs.y, mins.z);
	} else {
		ul = math::Vec3f(mins.x, maxs.y, mins.z);
		lr = math::Vec3f(maxs.x, mins.y, maxs.z);
	}
	const math::Vec3f vecs[] = {ll, ul, ur, lr};
	// lower left (0), upper left (1), upper right (2)
	// lower left (0), upper right (2), lower right (3)
	const uint32_t indices[] = {0, 1, 2, 0, 2, 3, 2, 1, 0, 3, 2, 0};
	shapeBuilder.clear();
	shapeBuilder.setColor(color);
	shapeBuilder.geom(vecs, lengthof(vecs), indices, lengthof(indices));
}

}
