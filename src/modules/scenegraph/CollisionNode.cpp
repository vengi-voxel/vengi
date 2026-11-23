/**
 * @file
 */

#include "CollisionNode.h"
#include "math/Mat4f.h"
#include "math/Vec4f.h"
#include "math/Vec3f.h"

namespace scenegraph {

CollisionNode::CollisionNode(const voxel::RawVolume *v, const math::Mat4f &m) : volume(v), worldToModel(m) {
}

math::Vec3f CollisionNode::calcModelSpace(const math::Vec3f &worldPos) const {
	const math::Mat4f model = worldToModel;
	return math::Vec3f(model * math::Vec4f(worldPos, 1.0f));
}

} // namespace scenegraph
