/**
 * @file
 */

#include "math/AABB.h"
#include "math/OBB.h"

#include "scenegraph/FrameTransform.h"
#include "voxel/Region.h"

namespace scenegraph {

math::OBBF toOBB(bool sceneMode, const voxel::Region &region, const glm::vec3 &normalizedPivot,
					   const scenegraph::FrameTransform &transform);
math::AABB<float> toAABB(const voxel::Region &region);
math::AABB<float> toAABB(const math::OBBF &obb);
voxel::Region toRegion(const math::AABB<float> &aabb);
voxel::Region toRegion(const math::OBBF &obb);

} // namespace scenegraph
