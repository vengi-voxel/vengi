/**
 * @file
 */

#include "BiomeManager.h"
#include "noise/Noise.h"
#include "commonlua/LUAFunctions.h"
#include "noise/PoissonDiskDistribution.h"
#include <utility>

namespace voxel {

static const Biome& getDefault() {
	static const Biome biome(VoxelType::Grass, getMaterialIndices(VoxelType::Grass), 0, MAX_MOUNTAIN_HEIGHT, 0.5f, 0.5f, false);
	return biome;
}

int Biome::calcTreeDistribution() const {
	int distribution = 100;
	if (temperature > 0.7f || humidity < 0.2f) {
		distribution = 150;
	} else if (temperature > 0.9f || humidity < 0.1f) {
		distribution = 200;
	}
	return distribution;
}

int Biome::calcCloudDistribution() const {
	int distribution = 150;
	if (temperature > 0.7f || humidity < 0.2f) {
		distribution = 200;
	} else if (temperature > 0.9f || humidity < 0.1f) {
		distribution = 250;
	}
	return distribution;
}

int Biome::calcPlantDistribution() const {
	int distribution = 30;
	if (temperature > 0.7f || humidity < 0.2f) {
		distribution = 50;
	} else if (temperature > 0.9f || humidity < 0.1f) {
		distribution = 100;
	}
	return distribution;
}

BiomeManager::BiomeManager() {
}

BiomeManager::~BiomeManager() {
}

bool BiomeManager::init(const std::string& luaString) {
	luaL_Reg luaAddBiome = { "addBiome", [] (lua_State* l) -> int {
		BiomeManager* biomeMgr = lua::LUA::globalData<BiomeManager>(l, "MGR");
		const int lower = luaL_checkinteger(l, 1);
		const int upper = luaL_checkinteger(l, 2);
		const float humidity = luaL_checknumber(l, 3);
		const float temperature = luaL_checknumber(l, 4);
		const char* voxelType = luaL_checkstring(l, 5);
		const bool underGround = clua_optboolean(l, 6, false);
		for (int j = (int)voxel::VoxelType::Air + 1; j < (int)voxel::VoxelType::Max; ++j) {
			if (strcmp(voxel::VoxelTypeStr[j], voxelType) != 0) {
				continue;
			}
			if (biomeMgr->addBiome(lower, upper, humidity, temperature, VoxelType(j), underGround)) {
				lua_pushboolean(l, 0);
			} else {
				lua_pushboolean(l, 1);
			}
			return 1;
		}
		lua_pushboolean(l, 0);
		return 1;
	}};

	lua::LUA lua;
	std::vector<luaL_Reg> funcs;
	funcs.push_back(luaAddBiome);
	funcs.push_back({ nullptr, nullptr });
	lua.newGlobalData<BiomeManager>("MGR", this);
	lua.reg("BiomeManager", &funcs.front());
	if (!lua.load(luaString)) {
		Log::error("Could not load lua script. Failed with error: %s", lua.error().c_str());
		return false;
	}
	if (!lua.execute("initBiomes")) {
		Log::error("Could not execute lua script. Failed with error: %s", lua.error().c_str());
		return false;
	}

	// TODO: init city gradients

	return !bioms.empty();
}

bool BiomeManager::addBiome(int lower, int upper, float humidity, float temperature, VoxelType type, bool underGround) {
	if (lower > upper) {
		return false;
	}
	const MaterialColorIndices& indices = getMaterialIndices(type);
	bioms.emplace_back(type, indices, int16_t(lower), int16_t(upper), humidity, temperature, underGround);
	return true;
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
	core_trace_scoped(BiomeGetBiome);

	struct Last {
		int x = 0;
		int z = 0;
		float humidity = -1.0f;
		float temperature = -1.0f;
		bool underground = false;
	};

	thread_local Last last;
	float humidity;
	float temperature;

	if (last.humidity > -1.0f && last.x == pos.x && last.z == pos.z && last.underground == underground) {
		humidity = last.humidity;
		temperature = last.temperature;
	} else {
		last.humidity = humidity = getHumidity(pos.x, pos.z);
		last.temperature = temperature = getTemperature(pos.x, pos.z);
		last.x = pos.x;
		last.z = pos.z;
		last.underground = underground;
	}

	const Biome *biomeBestMatch = &getDefault();
	float distMin = std::numeric_limits<float>::max();

	core_trace_scoped(BiomeGetBiomeLoop);
	for (const Biome& biome : bioms) {
		if (pos.y > biome.yMax || pos.y < biome.yMin || biome.underground != underground) {
			continue;
		}
		const float dTemperature = temperature - biome.temperature;
		const float dHumidity = humidity - biome.humidity;
		const float dist = (dTemperature * dTemperature) + (dHumidity * dHumidity);
		if (dist < distMin) {
			biomeBestMatch = &biome;
			distMin = dist;
		}
	}
	return biomeBestMatch;
}

void BiomeManager::distributePointsInRegion(const char *type, const Region& region, std::vector<glm::vec2>& positions, core::Random& random, int border, float distribution) const {
	std::vector<glm::vec2> initialSet;
	const glm::ivec3& randomPos = region.getRandomPosition(random);
	initialSet.push_back(glm::vec2(randomPos.x, randomPos.z));
	positions = noise::poissonDiskDistribution(distribution, region.rect(border), initialSet);
	Log::debug("%i %s positions in region (%i,%i,%i)/(%i,%i,%i) with border: %i", (int)positions.size(), type,
			region.getLowerX(), region.getLowerY(), region.getLowerZ(),
			region.getUpperX(), region.getUpperY(), region.getUpperZ(), border);
	for (const glm::vec2& pos : positions) {
		Log::debug("[+] %s pos: (%i:%i)", type, (int)pos.x, (int)pos.y);
	}
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
	if (getCityGradient(pos) < 0.4f) {
		return 1;
	}
	return 0;
}

float BiomeManager::getCityGradient(const glm::ivec3& pos) const {
	// TODO:
	return 1.0f;
}

bool BiomeManager::hasCity(const glm::ivec3& pos) const {
	core_trace_scoped(BiomeHasCity);
	return getCityGradient(pos) < 0.4f;
}

}
