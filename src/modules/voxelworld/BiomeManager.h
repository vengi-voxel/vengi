/**
 * @file
 */

#pragma once

#include "core/Trace.h"
#include "Biome.h"
#include "noise/Noise.h"
#include "TreeType.h"
#include <glm/glm.hpp>

namespace math {
class Random;
}

namespace voxel {
class Region;
}

namespace voxelworld {

enum class ZoneType {
	City,

	Max
};

/**
 * @brief A zone with a special meaning that might have influence on terrain generation.
 */
class Zone {
private:
	glm::ivec3 _pos;
	float _radius;
	ZoneType _type;
public:
	Zone(const glm::ivec3& pos, float radius, ZoneType type);
	const glm::ivec3& pos() const;
	float radius() const;
	ZoneType type() const;
};

inline ZoneType Zone::type() const {
	return _type;
}

inline const glm::ivec3& Zone::pos() const {
	return _pos;
}

inline float Zone::radius() const {
	return _radius;
}

class BiomeManager {
private:
	std::vector<Biome*> _bioms;
	std::vector<Zone*> _zones[int(ZoneType::Max)];
	const Biome* _defaultBiome = nullptr;
	void distributePointsInRegion(const voxel::Region& region, std::vector<glm::vec2>& positions, math::Random& random, int border, float distribution) const;
	noise::Noise _noise;

public:
	BiomeManager();
	~BiomeManager();

	static const float MinCityHeight;

	void shutdown();
	bool init(const std::string& luaString);

	Biome* addBiome(int lower, int upper, float humidity, float temperature, voxel::VoxelType type, bool underGround = false);

	// this lookup must be really really fast - it is executed once per generated voxel
	// iterating in y direction is fastest, because the last biome is cached on a per-thread-basis
	inline voxel::Voxel getVoxel(const glm::ivec3& pos, bool underground = false) const {
		core_trace_scoped(BiomeGetVoxel);
		const Biome* biome = getBiome(pos, underground);
		return biome->voxel();
	}

	inline voxel::Voxel getVoxel(int x, int y, int z, bool underground = false) const {
		return getVoxel(glm::ivec3(x, y, z), underground);
	}

	bool hasCactus(const glm::ivec3& pos) const;
	bool hasTrees(const glm::ivec3& pos) const;
	bool hasCity(const glm::ivec3& pos) const;
	bool hasClouds(const glm::ivec3& pos) const;
	bool hasPlants(const glm::ivec3& pos) const;

	void addZone(const glm::ivec3& pos, float radius, ZoneType type);
	const Zone* getZone(const glm::ivec3& pos, ZoneType type) const;
	const Zone* getZone(const glm::ivec2& pos, ZoneType type) const;
	int getCityDensity(const glm::ivec2& pos) const;
	float getCityMultiplier(const glm::ivec2& pos, int* targetHeight = nullptr) const;
	void getTreeTypes(const voxel::Region& region, std::vector<TreeType>& treeTypes) const;
	void getTreePositions(const voxel::Region& region, std::vector<glm::vec2>& positions, math::Random& random, int border) const;
	void getPlantPositions(const voxel::Region& region, std::vector<glm::vec2>& positions, math::Random& random, int border) const;
	void getCloudPositions(const voxel::Region& region, std::vector<glm::vec2>& positions, math::Random& random, int border) const;

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
