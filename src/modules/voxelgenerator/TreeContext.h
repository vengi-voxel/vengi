/**
 * @file
 */

#pragma once

#include "TreeType.h"
#include <glm/fwd.hpp>
#include <glm/vec3.hpp>

namespace voxelgenerator {

/**
 * @brief Base struct for all trees
 */
struct TreeConfig {
	unsigned int seed = 0;
	TreeType type = TreeType::Dome;
	glm::ivec3 pos { 0 };				/**< the position of the trunk bottom center */

	int trunkStrength = 2;
	int trunkHeight = 10;				/**< The height of the trunk - it's basically also the height of the tree */

	int leavesWidth = 8;				/**< the leave shape width */
	int leavesHeight = 16;				/**< the leave shape height - counting downward from the trunk top */
	int leavesDepth = 8;				/**< the leave shape depth */
};

struct TreeEllipsis : TreeConfig {
};

struct TreeBranchEllipsis : TreeConfig {
	int branchLength = 5;
	int branchHeight = 2;
};

struct TreeCone : TreeConfig {
};

struct TreePalm : TreeConfig {
	int branchSize = 5;
	int trunkWidth = 6;
	int trunkDepth = 3;
	float branchFactor = 0.95f;			/**< Defines how fast the branches get smaller */
	float trunkFactor = 0.8f;
	int branches = 6;					/**< How many branches/leaves */
	int branchControlOffset = 10;		/**< The control offset for the bezier curve of the palm leave */
	int trunkControlOffset = 10;		/**< The control offset for the bezier curve of the palm trunk */
	int randomLeavesHeightOffset = 3;
};

struct TreeFir : TreeConfig {
	TreeFir() {
		trunkHeight = 30;
	}
	int branches = 12;
	float w = 1.3f;
	int amount = 3;
	int stepHeight = 10;
	int branchStrength = 1;
	int branchDownwardOffset = 4;
	float branchPositionFactor = 1.8;
};

struct TreePine : TreeConfig {
	TreePine() {
		trunkHeight = 30;
		leavesHeight = 20;
		leavesDepth = 14;
		leavesWidth = 14;
	}
	int startWidth = 2;
	int startDepth = 2;
	int singleLeafHeight = 2;
	int singleStepDelta = 1;
};

struct TreeDome : TreeConfig {
	int branches = 6;
};

struct TreeDomeHanging : TreeDome {
	int hangingLeavesLengthMin = 4;
	int hangingLeavesLengthMax = 8;
	int hangingLeavesThickness = 1;
};

struct TreeCube : TreeConfig {
};

struct TreeSpaceColonization : TreeConfig {
	int branchSize = 5;
	float trunkFactor = 0.8f;
};

/**
 * @brief Context to create a tree.
 */
struct TreeContext {
	union {
		TreeConfig cfg;
		TreeEllipsis ellipsis;
		TreeBranchEllipsis branchellipsis;
		TreePalm palm;
		TreeCone cone;
		TreeFir fir;
		TreePine pine;
		TreeDome dome;
		TreeDomeHanging domehanging;
		TreeCube cube;
		TreeSpaceColonization spacecolonization;
	};

	TreeContext() {}
};

}
