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
	FIR,
	LSYSTEM,
	MAX
};

static const char *TreeTypeStr[] = {
	"DOME",
	"CONE",
	"ELLIPSIS",
	"CUBE",
	"PINE",
	"FIR",
	"LSYSTEM"
};
static_assert(SDL_arraysize(TreeTypeStr) == (int)TreeType::MAX, "TreeType and TreeTypeStr didn't match");

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
	Region _chunkRegion;
public:
	Region region;
	Region maxRegion = Region::MaxRegion;

	TerrainContext(PagedVolume* voxelStorage, PagedVolume::Chunk* chunk) :
			_voxelStorage(voxelStorage), _chunk(chunk) {
		_chunkRegion = chunk->getRegion();
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

	inline bool setVoxel(const glm::ivec3& pos, const Voxel& voxel) {
		return setVoxel(pos.x, pos.y, pos.z, voxel);
	}

	inline bool canGetVoxel(int x, int y, int z) const {
		return maxRegion.containsPoint(x, y, z);
	}

	inline const Voxel& getVoxel(const glm::ivec3& pos) const {
		return getVoxel(pos.x, pos.y, pos.z);
	}

	inline const Voxel& getVoxel(int x, int y, int z) const {
		if (_chunkRegion.containsPoint(x, y, z)) {
			core_assert(_chunk != nullptr);
			return _chunk->getVoxel(x - _chunkRegion.getLowerX(), y - _chunkRegion.getLowerY(), z - _chunkRegion.getLowerZ());
		}
		core_assert_msg(maxRegion.containsPoint(x, y, z), "the accessed voxel exceeds the max bounds of %i:%i:%i/%i:%i:%i (voxel was at %i:%i:%i)",
				maxRegion.getLowerX(), maxRegion.getLowerY(), maxRegion.getLowerZ(), maxRegion.getUpperX(), maxRegion.getUpperY(), maxRegion.getUpperZ(), x, y, z);
		core_assert(_voxelStorage != nullptr);
		return _voxelStorage->getVoxel(x, y, z);
	}

	inline bool setVoxel(int x, int y, int z, const Voxel& voxel) {
		if (_chunkRegion.containsPoint(x, y, z)) {
			core_assert(_chunk != nullptr);
			_chunk->setVoxel(x - _chunkRegion.getLowerX(), y - _chunkRegion.getLowerY(), z - _chunkRegion.getLowerZ(), voxel);
			return true;
		} else if (maxRegion.containsPoint(x, y, z)) {
			core_assert(_voxelStorage != nullptr);
			_voxelStorage->setVoxel(x, y, z, voxel);
			return true;
		}
		return false;
	}

	inline bool setVoxels(int x, int z, const Voxel* voxels, int amount) {
		if (_chunkRegion.containsPoint(x, 0, z)) {
			// first part goes into the chunk
			const int w = _chunkRegion.getWidthInVoxels();
			_chunk->setVoxels(x - _chunkRegion.getLowerX(), z - _chunkRegion.getLowerZ(), voxels, std::min(w, amount));
			amount -= w;
			if (amount > 0) {
				// everything else goes into the volume
				core_assert(_voxelStorage != nullptr);
				_voxelStorage->setVoxels(x, z, voxels + w, amount);
			}
			return true;
		} else if (maxRegion.containsPoint(x, 0, z)) {
			// TODO: add region/chunk support here, too
			core_assert(_voxelStorage != nullptr);
			_voxelStorage->setVoxels(x, z, voxels, amount);
			return true;
		}
		return false;
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
