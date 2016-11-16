/**
 * @file
 */

#pragma once

#include "voxel/polyvox/PagedVolume.h"
#include "voxel/polyvox/Region.h"
#include "voxel/polyvox/PagedVolumeWrapper.h"
#include <unordered_set>

namespace voxel {

enum class TreeType : int32_t {
	Dome,
	DomeHangingLeaves,
	Cone,
	Ellipsis,
	BranchesEllipsis,
	Cube,
	CubeSideCubes,
	Pine,
	Fir,
	Max
};

/**
 * @brief Cuts the given world coordinate down to mesh tile vectors
 */
static inline glm::ivec3 getGridPosForSize(const glm::ivec3& pos, float size) {
	const int x = glm::floor(pos.x / size);
	const int y = glm::floor(pos.y / size);
	const int z = glm::floor(pos.z / size);
	return glm::ivec3(x, y, z);
}

/**
 * @return the mins of the grid that the given position is in.
 * @param[in] pos The pos that is converted to the grid mins
 * @param[in] size The grid size that is used to calculate the mins
 */
static inline glm::ivec3 getGridBoundaryPos(const glm::ivec3& pos, int size) {
	const glm::ivec3& chunkPos = getGridPosForSize(pos, size);
	return glm::ivec3(chunkPos.x * size, 0, chunkPos.z * size);
}

/**
 * @brief Context to create a tree.
 */
struct TreeContext {
	TreeType type = TreeType::Dome;
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

struct WorldContext {
	int landscapeNoiseOctaves = 1;
	float landscapeNoisePersistence = 0.1f;
	float landscapeNoiseFrequency = 0.005f;
	float landscapeNoiseAmplitude = 0.6f;

	int caveNoiseOctaves = 1;
	float caveNoisePersistence = 0.1f;
	float caveNoiseFrequency = 0.05f;
	float caveNoiseAmplitude = 0.1f;
	float caveDensityThreshold = 0.83f;

	int mountainNoiseOctaves = 2;
	float mountainNoisePersistence = 0.3f;
	float mountainNoiseFrequency = 0.00075f;
	float mountainNoiseAmplitude = 0.5f;
};

}
