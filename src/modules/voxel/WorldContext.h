/**
 * @file
 */

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

/**
 * @brief Cuts the given world coordinate down to mesh tile vectors
 */
static inline glm::ivec3 getChunkPosForSize(const glm::ivec3& pos, float size) {
	const int x = glm::floor(pos.x / size);
	const int y = glm::floor(pos.y / size);
	const int z = glm::floor(pos.z / size);
	return glm::ivec3(x, y, z);
}

static inline glm::ivec3 getGridPosForSize(const glm::ivec3& pos, int size) {
	const glm::ivec3& chunkPos = getChunkPosForSize(pos, size);
	return glm::ivec3(chunkPos.x * size, 0, chunkPos.z * size);
}

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

struct GridPosHashEquals {
	int _chunkSize;

	GridPosHashEquals(int chunkSize) :
			_chunkSize(chunkSize) {
	}

	size_t operator()(const glm::ivec3& k) const {
		const glm::ivec3 g = getGridPosForSize(k, _chunkSize);
		// TODO: find a better hash function - we have a lot of collisions here
		return std::hash<int>()(g.x) ^ std::hash<int>()(g.y) ^ std::hash<int>()(g.z);
	}

	bool operator()(const glm::ivec3& a, const glm::ivec3& b) const {
		const glm::ivec3 ag = getGridPosForSize(a, _chunkSize);
		const glm::ivec3 bg = getGridPosForSize(b, _chunkSize);
		return ag == bg;
	}
};

typedef std::unordered_set<glm::ivec3, IVec3HashEquals> PositionSet;

class TerrainContext {
private:
	PagedVolume* _voxelStorage;
	PagedVolume::Chunk* _chunk;
public:
	Region region;
	PositionSet dirty;

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
			dirty.insert(glm::ivec3(x, y, z));
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
				dirty.insert(glm::ivec3(x, 0, z));
				_voxelStorage->setVoxels(x, z, voxels + w, amount);
			}
		} else {
			// TODO: add region/chunk support here, too
			core_assert(_voxelStorage != nullptr);
			dirty.insert(glm::ivec3(x, 0, z));
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
