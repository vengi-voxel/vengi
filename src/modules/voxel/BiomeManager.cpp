/**
 * @file
 */

#include "BiomeManager.h"
#include "noise/Noise.h"
#include "commonlua/LUAFunctions.h"

namespace voxel {

static const Biome& getDefault() {
	static const Biome biome(VoxelType::Grass, getMaterialIndices(VoxelType::Grass), 0, MAX_MOUNTAIN_HEIGHT, 0.5f, 0.5f, false);
	return biome;
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
	};

	thread_local Last last;
	float humidity;
	float temperature;

	if (last.humidity > -1.0f && last.x == pos.x && last.z == pos.z) {
		humidity = last.humidity;
		temperature = last.temperature;
	} else {
		last.humidity = humidity = getHumidity(pos.x, pos.z);
		last.temperature = temperature = getTemperature(pos.x, pos.z);
		last.x = pos.x;
		last.z = pos.z;
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

int BiomeManager::getAmountOfTrees(const Region& region) const {
	core_trace_scoped(BiomeGetAmountOfTrees);
	const glm::ivec3&pos = region.getCentre();
	const Biome* biome = getBiome(pos);
	const int maxDim = region.getDepthInCells();
	if (biome->temperature > 0.7f || biome->humidity < 0.2f) {
		return maxDim / 16;
	}
	if (biome->temperature > 0.9f || biome->humidity < 0.1f) {
		return maxDim / 32;
	}
	return maxDim / 6;
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
	return biome->temperature > 0.9f || biome->humidity < 0.1f;
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
	if (biome->temperature > 0.9f || biome->humidity < 0.1f) {
		return false;
	}
	return biome->temperature > 0.3f && biome->humidity > 0.3f;
}

bool BiomeManager::hasClouds(const glm::ivec3& pos) const {
	core_trace_scoped(BiomeHasClouds);
	if (pos.y <= MAX_MOUNTAIN_HEIGHT) {
		return false;
	}
	const Biome* biome = getBiome(pos);
	return biome->humidity >= 0.5f;
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
