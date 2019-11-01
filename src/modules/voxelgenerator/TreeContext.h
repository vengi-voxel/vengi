/**
 * @file
 */

#pragma once

#include "voxelworld/TreeType.h"
#include <glm/fwd.hpp>
#include <glm/vec3.hpp>

namespace voxelgenerator {

/**
 * @brief Context to create a tree.
 */
struct TreeContext {
	voxelworld::TreeType type = voxelworld::TreeType::Dome;
	int trunkHeight = 24;	/**< The height of the trunk - it's basically also the height of the tree */
	int trunkWidth = 2;
	int leavesWidth = 8;	/**< the leave shape width */
	int leavesHeight = 16;	/**< the leave shape height - counting downward from the trunk top */
	int leavesDepth = 8;	/**< the leave shape depth */
	glm::ivec3 pos;			/**< the position of the trunk bottom center */

	inline int treeBottom() const {
		return pos.y;
	}

	inline int treeTop() const {
		return treeBottom() + trunkHeight;
	}

	inline int leavesBottom() const {
		return leavesTop() - leavesHeight;
	}

	inline int leavesTop() const {
		return treeTop();
	}

	inline glm::ivec3 leavesTopV() const {
		return glm::ivec3(pos.x, leavesTop(), pos.z);
	}

	inline glm::ivec3 trunkTopV() const {
		return glm::ivec3(pos.x, treeTop(), pos.z);
	}

	inline int leavesCenter() const {
		return leavesTop() - leavesHeight / 2;
	}

	inline int trunkCenter() const {
		return treeBottom() + trunkHeight / 2;
	}

	inline glm::ivec3 leavesCenterV() const {
		return glm::ivec3(pos.x, leavesCenter(), pos.z);
	}

	inline glm::ivec3 trunkCenterV() const {
		return glm::ivec3(pos.x, trunkCenter(), pos.z);
	}
};

}
