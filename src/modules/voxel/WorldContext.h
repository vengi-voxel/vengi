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
	LSYSTEM,
	MAX
};

static const char *TreeTypeStr[] = {
	"DOME",
	"CONE",
	"ELLIPSIS",
	"CUBE",
	"PINE",
	"LSYSTEM"
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

class TerrainContext {
private:
	PagedVolume* _voxelStorage;
public:
	TerrainContext(PagedVolume* voxelStorage) :
			_voxelStorage(voxelStorage) {
	}

	Region region;
	PositionSet dirty;

	inline void setVoxel(const glm::ivec3& pos, const Voxel& voxel) {
		setVoxel(pos.x, pos.y, pos.z, voxel);
	}

	inline const Voxel& getVoxel(const glm::ivec3& pos) const {
		return getVoxel(pos.x, pos.y, pos.z);
	}

	inline const Voxel& getVoxel(int x, int y, int z) const {
		core_assert(_voxelStorage != nullptr);
		return _voxelStorage->getVoxel(x, y, z);
	}

	inline void setVoxel(int x, int y, int z, const Voxel& voxel) {
		if (_voxelStorage == nullptr)
			return;
		_voxelStorage->setVoxel(x, y, z, voxel);
	}

	inline void setVoxels(int x, int z, const Voxel* voxels, int amount) {
		if (_voxelStorage == nullptr)
			return;
		_voxelStorage->setVoxels(x, z, voxels, amount);
	}
};

struct WorldContext {
	int landscapeNoiseOctaves = 1;
	float landscapeNoisePersistence = 0.1f;
	float landscapeNoiseFrequency = 0.01f;
	float landscapeNoiseAmplitude = 0.5f;

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
