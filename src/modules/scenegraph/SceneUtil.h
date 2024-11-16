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

} // namespace scenegraph
