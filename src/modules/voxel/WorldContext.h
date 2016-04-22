#pragma once

#include "polyvox/PagedVolume.h"
#include "polyvox/Region.h"
#include <unordered_set>

namespace voxel {

enum class TreeType : int32_t {
	DOME,
	CONE,
	ELLIPSIS,
	CUBE,
	PINE,
	MAX
};

static const char *TreeTypeStr[] = {
	"DOME",
	"CONE",
	"ELLIPSIS",
	"CUBE",
	"PINE"
};
static_assert(SDL_arraysize(TreeTypeStr) == (int)TreeType::MAX, "TreeType and TreeTypeStr didn't match");

struct TreeContext {
	TreeType type = TreeType::DOME;
	int trunkHeight = 6;
	int trunkWidth = 2;
	int width = 10;
	int height = 10;
	int depth = 10;
	// y-level is automatically determined
	glm::ivec2 pos = glm::ivec2(0, 0);
};

struct IVec3HashEquals {
	size_t operator()(const glm::ivec3& k) const {
		// TODO: find a better hash function - we have a lot of collisions here
		return std::hash<int>()(k.x) ^ std::hash<int>()(k.y) ^ std::hash<int>()(k.z);
	}

	bool operator()(const glm::ivec3& a, const glm::ivec3& b) const {
		return a == b;
	}
};

typedef std::unordered_set<glm::ivec3, IVec3HashEquals> PositionSet;

struct TerrainContext {
	Region region;
	// if no chunk is given, the positions are defined in absolute world coordinates
	// otherwise they should be given in chunk coordinates.
	// if a chunk region is exceeded by a coordinate (which might be true for e.g. tree,
	// cloud or building generation) then the relative chunk coordinate is converted into
	// an absolute position in the world by taking the given region parameter into account
	PagedVolume::Chunk* chunk;
	PositionSet dirty;
};

}
