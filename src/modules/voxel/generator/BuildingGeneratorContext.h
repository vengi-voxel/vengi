/**
 * @file
 */

#pragma once

#include <glm/glm.hpp>

namespace voxel {

enum class BuildingType {
	Tower,
	House,

	Max
};

struct BuildingContext {
	int floors = 1;
	int floorHeight = 10;
	int floorDepth = 10;
	int floorWidth = 10;
	int windowWidth = 3;
	int windowHeight = 3;
	int doorWidth = 3;
	int doorHeight = 6;
	int roofHeight = 5;
	int wallOffset = 0;
	int wallStrength = 1;
	glm::ivec3 pos;			/**< lower left corner of the building */
};

}
