/**
 * @file
 */

#pragma once

#include "core/collection/DynamicArray.h"
#include "math/Mat4f.h"
#include "math/Vec3f.h"

namespace voxel {
class RawVolume;
}

namespace scenegraph {

/**
 * @brief Represents a node in the scene graph that can be collided with.
 */
struct CollisionNode {
	CollisionNode() = default;
	CollisionNode(const voxel::RawVolume *v, const math::Mat4f &m);
	// the volume data to check collisions against
	const voxel::RawVolume *volume = nullptr;
	// the inverse matrix includes the pivot translation and is used to transform world positions into model space for the given volume
	math::Mat4f worldToModel = math::Mat4f::identity();

	/**
	 * @brief Transforms a world position into model space.
	 * @param[in] worldPos The world position to transform.
	 * @return The transformed position in model space - this can be used to e.g. trace inside a volume.
	 */
	math::Vec3f calcModelSpace(const math::Vec3f &worldPos) const;
};

/**
 * @brief A collection of collision nodes.
 * @sa CollisionNode
 */
using CollisionNodes = core::DynamicArray<CollisionNode>;

} // namespace scenegraph
