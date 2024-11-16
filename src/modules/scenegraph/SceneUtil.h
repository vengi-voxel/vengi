/**
 * @file
 */

#include "math/AABB.h"
#include "math/OBB.h"

#include "scenegraph/FrameTransform.h"
#include "voxel/Region.h"

namespace scenegraph {

math::OBB<float> toOBB(bool sceneMode, const voxel::Region &region, const glm::vec3 &normalizedPivot,
					   const scenegraph::FrameTransform &transform);
math::AABB<float> toAABB(const voxel::Region &region);
math::AABB<float> toAABB(const math::OBB<float> &obb);
voxel::Region toRegion(const math::AABB<float> &aabb);
voxel::Region toRegion(const math::OBB<float> &obb);

} // namespace scenegraph
