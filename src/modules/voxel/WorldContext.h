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
	PagedVolume::Chunk* _chunk;
public:
	TerrainContext(PagedVolume* voxelStorage, PagedVolume::Chunk* chunk) :
			_voxelStorage(voxelStorage), _chunk(chunk) {
	}

	void setChunk(PagedVolume::Chunk* chunk) {
		_chunk = chunk;
	}

	void setVoxelStorage(PagedVolume* voxelStorage) {
		_voxelStorage = voxelStorage;
	}

	inline PagedVolume::Chunk* getChunk() const {
		return _chunk;
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
		if (region.containsPoint(x, y, z)) {
			core_assert(_chunk != nullptr);
			return _chunk->getVoxel(x - region.getLowerX(), y - region.getLowerY(), z - region.getLowerZ());
		}
		core_assert(_voxelStorage != nullptr);
		return _voxelStorage->getVoxel(x, y, z);
	}

	inline void setVoxel(int x, int y, int z, const Voxel& voxel) {
		if (region.containsPoint(x, y, z)) {
			core_assert(_chunk != nullptr);
			_chunk->setVoxel(x - region.getLowerX(), y - region.getLowerY(), z - region.getLowerZ(), voxel);
		} else {
			core_assert(_voxelStorage != nullptr);
			_voxelStorage->setVoxel(x, y, z, voxel);
		}
	}

	inline void setVoxels(int x, int z, const Voxel* voxels, int amount) {
		if (region.containsPoint(x, 0, z)) {
			// first part goes into the chunk
			const int w = region.getWidthInVoxels();
			_chunk->setVoxels(x - region.getLowerX(), z - region.getLowerZ(), voxels, std::min(w, amount));
			amount -= w;
			if (amount > 0) {
				// everything else goes into the volume
				core_assert(_voxelStorage != nullptr);
				_voxelStorage->setVoxels(x, z, voxels + w, amount);
			}
		} else {
			// TODO: add region/chunk support here, too
			core_assert(_voxelStorage != nullptr);
			_voxelStorage->setVoxels(x, z, voxels, amount);
		}
	}
};

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
