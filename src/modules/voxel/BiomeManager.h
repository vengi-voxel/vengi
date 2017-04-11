/**
 * @file
 */

#pragma once

#include "core/Trace.h"
#include "Biome.h"
#include "TreeContext.h"
#include <glm/glm.hpp>

namespace core {
class Random;
}

namespace voxel {

class Region;

class BiomeManager {
private:
	std::vector<Biome*> _bioms;
	const Biome* _defaultBiome = nullptr;
	void distributePointsInRegion(const char *type, const Region& region, std::vector<glm::vec2>& positions, core::Random& random, int border, float distribution) const;

public:
	BiomeManager();
	~BiomeManager();

	bool init(const std::string& luaString);

	Biome* addBiome(int lower, int upper, float humidity, float temperature, VoxelType type, bool underGround = false);

	// this lookup must be really really fast - it is executed once per generated voxel
	// iterating in y direction is fastest, because the last biome is cached on a per-thread-basis
	inline Voxel getVoxel(const glm::ivec3& pos, bool underground = false) const {
		core_trace_scoped(BiomeGetVoxel);
		const Biome* biome = getBiome(pos, underground);
		return biome->voxel();
	}

	inline Voxel getVoxel(int x, int y, int z, bool underground = false) const {
		return getVoxel(glm::ivec3(x, y, z), underground);
	}

	bool hasCactus(const glm::ivec3& pos) const;
	bool hasTrees(const glm::ivec3& pos) const;
	bool hasCity(const glm::ivec3& pos) const;
	bool hasClouds(const glm::ivec3& pos) const;
	bool hasPlants(const glm::ivec3& pos) const;

	int getCityDensity(const glm::ivec3& pos) const;
	float getCityGradient(const glm::ivec3& pos) const;
	void getTreeTypes(const Region& region, std::vector<TreeType>& treeTypes) const;
	void getTreePositions(const Region& region, std::vector<glm::vec2>& positions, core::Random& random, int border) const;
	void getPlantPositions(const Region& region, std::vector<glm::vec2>& positions, core::Random& random, int border) const;
	void getCloudPositions(const Region& region, std::vector<glm::vec2>& positions, core::Random& random, int border) const;

	/**
	 * @return Humidity noise in the range [0-1]
	 */
	float getHumidity(int x, int z) const;
	/**
	 * @return Temperature noise in the range [0-1]
	 */
	float getTemperature(int x, int z) const;

	void setDefaultBiome(const Biome* biome);

	const Biome* getBiome(const glm::ivec3& pos, bool underground = false) const;
};

}
