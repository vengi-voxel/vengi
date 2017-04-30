/**
 * @file
 */

#include "BiomeManager.h"
#include "noise/Noise.h"
#include "noise/PoissonDiskDistribution.h"
#include "core/Random.h"
#include "core/Common.h"
#include "core/GLM.h"
#include "Constants.h"
#include "polyvox/Region.h"
#include "MaterialColor.h"
#include "BiomeLUAFunctions.h"
#include "commonlua/LUAFunctions.h"
#include <utility>

namespace voxel {

static const Biome& getDefaultBiome() {
	static const Biome biome(VoxelType::Grass, getMaterialIndices(VoxelType::Grass), 0, MAX_MOUNTAIN_HEIGHT, 0.5f, 0.5f, false);
	return biome;
}

Zone::Zone(const glm::ivec3& pos, float radius, ZoneType type) :
		_pos(pos), _radius(radius), _type(type) {
}

BiomeManager::BiomeManager() {
}

BiomeManager::~BiomeManager() {
	for (const Biome* biome : _bioms) {
		delete biome;
	}
	_bioms.clear();
	for (int i = 0; i < std::enum_value(ZoneType::Max); ++i) {
		for (const Zone* zone : _zones[i]) {
			delete zone;
		}
		_zones[i].clear();
	}
}

bool BiomeManager::init(const std::string& luaString) {
	_defaultBiome = &getDefaultBiome();

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

	return !_bioms.empty();
}

Biome* BiomeManager::addBiome(int lower, int upper, float humidity, float temperature, VoxelType type, bool underGround) {
	core_assert_msg(_defaultBiome != nullptr, "BiomeManager is not yet initialized");
	if (lower > upper) {
		return nullptr;
	}
	const MaterialColorIndices& indices = getMaterialIndices(type);
	Biome* biome = new Biome(type, indices, int16_t(lower), int16_t(upper), humidity, temperature, underGround);
	_bioms.push_back(biome);
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

	if (last.humidity > -1.0f && last.pos == pos && last.underground == underground) {
		humidity = last.humidity;
		temperature = last.temperature;
	} else {
		last.humidity = humidity = getHumidity(pos.x, pos.z);
		last.temperature = temperature = getTemperature(pos.x, pos.z);
		last.pos = pos;
		last.underground = underground;
	}

	const Biome *biomeBestMatch = _defaultBiome;
	float distMin = std::numeric_limits<float>::max();

	core_trace_scoped(BiomeGetBiomeLoop);
	for (const Biome* biome : _bioms) {
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
	return biomeBestMatch;
}

void BiomeManager::distributePointsInRegion(const char *type, const Region& region, std::vector<glm::vec2>& positions, core::Random& random, int border, float distribution) const {
	std::vector<glm::vec2> initialSet;
	voxel::Region shrinked = region;
	shrinked.shrink(border);
	const glm::ivec3& randomPos = shrinked.getRandomPosition(random);
	initialSet.push_back(glm::vec2(randomPos.x, randomPos.z));
	positions = noise::poissonDiskDistribution(distribution, shrinked.rect(), initialSet);
	Log::debug("%i %s positions in region (%i,%i,%i)/(%i,%i,%i) with border: %i", (int)positions.size(), type,
			region.getLowerX(), region.getLowerY(), region.getLowerZ(),
			region.getUpperX(), region.getUpperY(), region.getUpperZ(), border);
	for (const glm::vec2& pos : positions) {
		Log::debug("[+] %s pos: (%i:%i)", type, (int)pos.x, (int)pos.y);
	}
}

void BiomeManager::getTreeTypes(const Region& region, std::vector<TreeType>& treeTypes) const {
	const glm::ivec3& pos = region.getCentre();
	const Biome* biome = getBiome(pos);
	treeTypes = biome->treeTypes();
}

void BiomeManager::getTreePositions(const Region& region, std::vector<glm::vec2>& positions, core::Random& random, int border) const {
	core_trace_scoped(BiomeGetTreePositions);
	const glm::ivec3& pos = region.getCentre();
	if (!hasTrees(pos)) {
		return;
	}
	const Biome* biome = getBiome(pos);
	distributePointsInRegion("tree", region, positions, random, border, biome->treeDistribution);
}

void BiomeManager::getPlantPositions(const Region& region, std::vector<glm::vec2>& positions, core::Random& random, int border) const {
	core_trace_scoped(BiomeGetPlantPositions);
	const glm::ivec3& pos = region.getCentre();
	if (!hasPlants(pos)) {
		return;
	}
	const Biome* biome = getBiome(pos);
	distributePointsInRegion("plant", region, positions, random, border, biome->plantDistribution);
}

void BiomeManager::getCloudPositions(const Region& region, std::vector<glm::vec2>& positions, core::Random& random, int border) const {
	core_trace_scoped(BiomeGetCloudPositions);
	glm::ivec3 pos = region.getCentre();
	pos.y = region.getUpperY();
	if (!hasClouds(pos)) {
		return;
	}

	const Biome* biome = getBiome(pos);
	distributePointsInRegion("cloud", region, positions, random, border, biome->cloudDistribution);
}

bool BiomeManager::hasCactus(const glm::ivec3& pos) const {
	core_trace_scoped(BiomeHasCactus);
	if (pos.y < MAX_WATER_HEIGHT) {
		return false;
	}
	const Biome* biome = getBiome(pos);
	if (!isSand(biome->type)) {
		return false;
	}
	return biome->hasCactus();
}

bool BiomeManager::hasTrees(const glm::ivec3& pos) const {
	core_trace_scoped(BiomeHasTrees);
	if (pos.y < MAX_WATER_HEIGHT) {
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
	if (pos.y <= MAX_MOUNTAIN_HEIGHT) {
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

int BiomeManager::getCityDensity(const glm::ivec3& pos) const {
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
		const float distance = glm::distance2(pos, z->pos());
		if (distance < glm::pow(z->radius(), 2)) {
			return z;
		}
	}
	return nullptr;
}

float BiomeManager::getCityMultiplier(const glm::ivec3& pos) const {
	const Zone* zone = getZone(pos, ZoneType::City);
	if (zone == nullptr) {
		return 1.0f;
	}
	const glm::vec3& zonePos = zone->pos();
	const glm::vec3 dist(pos.x - zonePos.x, 0.0f, pos.z - zonePos.z);
	return glm::clamp(glm::length(dist) / zone->radius(), 0.01f, 1.0f);
}

bool BiomeManager::hasCity(const glm::ivec3& pos) const {
	core_trace_scoped(BiomeHasCity);
	return getZone(pos, ZoneType::City) != nullptr;
}

void BiomeManager::setDefaultBiome(const Biome* biome) {
	if (biome == nullptr) {
		biome = &getDefaultBiome();
	}
	_defaultBiome = biome;
}

}
