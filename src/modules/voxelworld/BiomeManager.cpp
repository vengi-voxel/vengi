/**
 * @file
 */

#include "BiomeManager.h"
#include "noise/Noise.h"
#include "noise/PoissonDiskDistribution.h"
#include "math/Random.h"
#include "core/Common.h"
#include "core/GLM.h"
#include "core/Log.h"
#include "noise/Simplex.h"
#include "voxel/Constants.h"
#include "voxel/Region.h"
#include "voxel/MaterialColor.h"
#include "BiomeLUAFunctions.h"
#include "commonlua/LUAFunctions.h"
#include <utility>

namespace voxelworld {

Zone::Zone(const glm::ivec3& pos, float radius, ZoneType type) :
		_pos(pos), _radius(radius), _type(type) {
}

const float BiomeManager::MinCityHeight = (voxel::MAX_WATER_HEIGHT + 1) / (float)(voxel::MAX_TERRAIN_HEIGHT - 1);

BiomeManager::BiomeManager() {
}

BiomeManager::~BiomeManager() {
	shutdown();
}

void BiomeManager::shutdown() {
	_noise.shutdown();
	_defaultBiome = nullptr;
	for (const Biome* biome : _biomes) {
		delete biome;
	}
	_biomes.clear();
	for (int i = 0; i < std::enum_value(ZoneType::Max); ++i) {
		for (const Zone* zone : _zones[i]) {
			delete zone;
		}
		_zones[i].clear();
	}
}

bool BiomeManager::init(const std::string& luaString) {
	if (!_noise.init()) {
		return false;
	}
	setDefaultBiome(nullptr);

	Log::debug("Minimum city height: %f", MinCityHeight);

	lua::LUA lua;
	lua.newGlobalData<BiomeManager>("MGR", this);
	const std::vector<luaL_Reg> funcs({
		{"addBiome", biomelua_addbiome},
		{"addCity", biomelua_addcity},
		{"setDefault", biomelua_setdefault},
		{nullptr, nullptr}
	});
	lua.reg("biomeMgr", &funcs.front());
	biomelua_biomeregister(lua.state());
	if (!lua.load(luaString)) {
		Log::error("Could not load lua script. Failed with error: %s", lua.error().c_str());
		return false;
	}
	if (!lua.execute("initBiomes")) {
		Log::error("Could not execute lua script. Failed with error: %s", lua.error().c_str());
		return false;
	}
	if (!lua.execute("initCities")) {
		Log::error("Could not execute lua script. Failed with error: %s", lua.error().c_str());
		return false;
	}

	return !_biomes.empty();
}

Biome* BiomeManager::addBiome(int lower, int upper, float humidity, float temperature,
		voxel::VoxelType type, int treeDistribution,
		int cloudDistribution, int plantDistribution, bool underGround) {
	core_assert_msg(_defaultBiome != nullptr, "BiomeManager is not yet initialized");
	if (lower > upper) {
		Log::warn("Failed to create biome, lower value is bigger than upper value");
		return nullptr;
	}
	if (getMaterialIndices(type).empty()) {
		Log::warn("Failed to create biome, could not find any material indices for type: %i", (int)type);
		return nullptr;
	}
	Biome* biome = new Biome(type, int16_t(lower), int16_t(upper),
			humidity, temperature, underGround, treeDistribution, cloudDistribution,
			plantDistribution);
	_biomes.push_back(biome);
	return biome;
}

float BiomeManager::getHumidity(int x, int z) const {
	core_trace_scoped(BiomeGetHumidity);
	const float frequency = 0.001f;
	const glm::vec2 noisePos(x * frequency, z * frequency);
	const float n = noise::noise(noisePos);
	return noise::norm(n);
}

float BiomeManager::getTemperature(int x, int z) const {
	core_trace_scoped(BiomeGetTemperature);
	const float frequency = 0.0001f;
	// TODO: apply y value
	// const float scaleY = pos.y / (float)MAX_HEIGHT;
	const glm::vec2 noisePos(x * frequency, z * frequency);
	const float n = noise::noise(noisePos);
	return noise::norm(n);
}

const Biome* BiomeManager::getBiome(const glm::ivec3& pos, bool underground) const {
	core_assert_msg(_defaultBiome != nullptr, "BiomeManager is not yet initialized");
	core_trace_scoped(BiomeGetBiome);

	struct Last {
		glm::ivec3 pos;
		float humidity = -1.0f;
		float temperature = -1.0f;
		bool underground = false;
	};

	thread_local Last last;
	float humidity;
	float temperature;

	if (last.humidity > -1.0f && last.pos.x == pos.x && last.pos.z == pos.z && last.underground == underground) {
		humidity = last.humidity;
		temperature = last.temperature;
	} else {
		last.humidity = humidity = getHumidity(pos.x, pos.z);
		last.temperature = temperature = getTemperature(pos.x, pos.z);
		last.pos = pos;
		last.underground = underground;
	}

	const Biome *biomeBestMatch = _defaultBiome;
	float distMin = (std::numeric_limits<float>::max)();

	{
	core_trace_scoped(BiomeGetBiomeLoop);
	for (const Biome* biome : _biomes) {
		if (pos.y > biome->yMax || pos.y < biome->yMin || biome->underground != underground) {
			continue;
		}
		const float dTemperature = temperature - biome->temperature;
		const float dHumidity = humidity - biome->humidity;
		const float dist = (dTemperature * dTemperature) + (dHumidity * dHumidity);
		if (dist < distMin) {
			biomeBestMatch = biome;
			distMin = dist;
		}
	}
	}
	return biomeBestMatch;
}

void BiomeManager::distributePointsInRegion(const voxel::Region& region,
		std::vector<glm::vec2>& positions, math::Random& random, int border, float distribution) const {
	std::vector<glm::vec2> initialSet;
	voxel::Region shrinked = region;
	shrinked.shrink(border);
	const glm::ivec3& randomPos = shrinked.getRandomPosition(random);
	initialSet.push_back(glm::vec2(randomPos.x, randomPos.z));
	positions = noise::poissonDiskDistribution(distribution, shrinked.rect(), initialSet);
}

const std::vector<const char*>& BiomeManager::getTreeTypes(const voxel::Region& region) const {
	const glm::ivec3& pos = region.getCentre();
	const Biome* biome = getBiome(pos);
	return biome->treeTypes();
}

void BiomeManager::getTreePositions(const voxel::Region& region,
		std::vector<glm::vec2>& positions, math::Random& random, int border) const {
	core_trace_scoped(BiomeGetTreePositions);
	const glm::ivec3& pos = region.getCentre();
	if (!hasTrees(pos)) {
		return;
	}
	const Biome* biome = getBiome(pos);
	distributePointsInRegion(region, positions, random, border, biome->treeDistance);
}

void BiomeManager::getPlantPositions(const voxel::Region& region,
		std::vector<glm::vec2>& positions, math::Random& random, int border) const {
	core_trace_scoped(BiomeGetPlantPositions);
	const glm::ivec3& pos = region.getCentre();
	if (!hasPlants(pos)) {
		return;
	}
	const Biome* biome = getBiome(pos);
	distributePointsInRegion(region, positions, random, border, biome->plantDistribution);
}

void BiomeManager::getCloudPositions(const voxel::Region& region, std::vector<glm::vec2>& positions, math::Random& random, int border) const {
	core_trace_scoped(BiomeGetCloudPositions);
	glm::ivec3 pos = region.getCentre();
	pos.y = region.getUpperY();
	if (!hasClouds(pos)) {
		return;
	}

	const Biome* biome = getBiome(pos);
	distributePointsInRegion(region, positions, random, border, biome->cloudDistribution);
}

bool BiomeManager::hasCactus(const glm::ivec3& pos) const {
	core_trace_scoped(BiomeHasCactus);
	if (pos.y < voxel::MAX_WATER_HEIGHT) {
		return false;
	}
	const Biome* biome = getBiome(pos);
	if (!voxel::isSand(biome->type)) {
		return false;
	}
	return biome->hasCactus();
}

bool BiomeManager::hasTrees(const glm::ivec3& pos) const {
	core_trace_scoped(BiomeHasTrees);
	if (pos.y < voxel::MAX_WATER_HEIGHT) {
		return false;
	}
	const Biome* biome = getBiome(pos);
	if (!isGrass(biome->type)) {
		return false;
	}
	if (biome->hasCactus()) {
		return false;
	}
	return biome->hasTrees();
}

bool BiomeManager::hasClouds(const glm::ivec3& pos) const {
	core_trace_scoped(BiomeHasClouds);
	if (pos.y <= voxel::MAX_MOUNTAIN_HEIGHT) {
		return false;
	}
	const Biome* biome = getBiome(pos);
	return biome->hasClouds();
}

bool BiomeManager::hasPlants(const glm::ivec3& pos) const {
	core_trace_scoped(BiomeHasPlants);
	// TODO:
	return hasTrees(pos);
}

int BiomeManager::getCityDensity(const glm::ivec2& pos) const {
	// TODO:
	if (getCityMultiplier(pos) < 0.4f) {
		return 1;
	}
	return 0;
}

void BiomeManager::addZone(const glm::ivec3& pos, float radius, ZoneType type) {
	_zones[std::enum_value(type)].push_back(new Zone(pos, radius, type));
}

const Zone* BiomeManager::getZone(const glm::ivec3& pos, ZoneType type) const {
	for (const Zone* z : _zones[std::enum_value(type)]) {
		const float distance = glm::distance2(glm::vec3(pos), glm::vec3(z->pos()));
		if (distance < glm::pow(z->radius(), 2)) {
			return z;
		}
	}
	return nullptr;
}

const Zone* BiomeManager::getZone(const glm::ivec2& pos, ZoneType type) const {
	const glm::vec3 p(pos.x, 0.0f, pos.y);
	for (const Zone* z : _zones[std::enum_value(type)]) {
		const glm::ivec3& zp = z->pos();
		const float distance = glm::distance2(p, glm::vec3(zp.x, 0.0f, zp.z));
		if (distance < glm::pow(z->radius(), 2)) {
			return z;
		}
	}
	return nullptr;
}

float BiomeManager::getCityMultiplier(const glm::ivec2& pos, int* targetHeight) const {
	const Zone* zone = getZone(pos, ZoneType::City);
	if (zone == nullptr) {
		return 1.0f;
	}
	const glm::ivec3& zonePos = zone->pos();
	const glm::vec3 dist(pos.x - zonePos.x, 0.0f, pos.y - zonePos.z);

	if (targetHeight != nullptr) {
		*targetHeight = voxel::MAX_WATER_HEIGHT + 2;
	}
	const float l = glm::length(dist);
	if (glm::abs(l) < glm::epsilon<float>()) {
		return 0.0f;
	}
	// near: 1 / (1000 /  0.1)^2
	// far:  1 / (1000 / 1000)^2
	const float v = 1.0f / glm::pow(zone->radius() / l, 2);
	return v;
}

bool BiomeManager::hasCity(const glm::ivec3& pos) const {
	core_trace_scoped(BiomeHasCity);
	return getZone(pos, ZoneType::City) != nullptr;
}

void BiomeManager::setDefaultBiome(const Biome* biome) {
	if (biome == nullptr) {
		static const Biome defaultBiome;
		biome = &defaultBiome;
	}
	_defaultBiome = biome;
}

}
