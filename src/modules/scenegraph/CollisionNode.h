/**
 * @file
 */

#pragma once

#include "core/collection/DynamicArray.h"
#include <glm/mat4x4.hpp>

namespace voxel {
class RawVolume;
}

namespace scenegraph {

/**
 * @brief Represents a node in the scene graph that can be collided with.
 */
struct CollisionNode {
	CollisionNode() = default;
	CollisionNode(const voxel::RawVolume *v, const glm::mat4 &m) : volume(v), worldToModel(m) {
	}
	// the volume data to check collisions against
	const voxel::RawVolume *volume = nullptr;
	// the inverse matrix includes the pivot translation and is used to transform world positions into model space for the given volume
	glm::mat4 worldToModel;

	/**
	 * @brief Transforms a world position into model space.
	 * @param[in] worldPos The world position to transform.
	 * @return The transformed position in model space - this can be used to e.g. trace inside a volume.
	 */
	glm::vec3 calcModelSpace(const glm::vec3 &worldPos) const {
		const glm::vec3 modelSpacePos(worldToModel * glm::vec4(worldPos, 1.0f));
		return modelSpacePos;
	}
};

/**
 * @brief A collection of collision nodes.
 * @sa CollisionNode
 */
using CollisionNodes = core::DynamicArray<CollisionNode>;

} // namespace scenegraph
