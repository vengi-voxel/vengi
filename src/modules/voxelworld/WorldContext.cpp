/**
 * @file
 */

#include "WorldContext.h"
#include "commonlua/LUA.h"
#include "core/Log.h"

namespace voxelworld {

#define CTX_LUA_FLOAT(name) name = lua.floatValue(#name, name)
#define CTX_LUA_INT(name) name = lua.intValue(#name, name)

WorldContext::WorldContext() :
	landscapeNoiseOctaves(1), landscapeNoiseLacunarity(0.1f), landscapeNoiseFrequency(0.005f), landscapeNoiseGain(0.6f),
	caveNoiseOctaves(1), caveNoiseLacunarity(0.1f), caveNoiseFrequency(0.05f), caveNoiseGain(0.1f), caveDensityThreshold(0.83f),
	mountainNoiseOctaves(2), mountainNoiseLacunarity(0.3f), mountainNoiseFrequency(0.00075f), mountainNoiseGain(0.5f) {
}

bool WorldContext::load(const std::string& luaString) {
	if (luaString.empty()) {
		return true;
	}
	lua::LUA lua;
	if (!lua.load(luaString)) {
		Log::error("Could not load lua script. Failed with error: %s", lua.error().c_str());
		return false;
	}

	CTX_LUA_INT(landscapeNoiseOctaves);
	CTX_LUA_FLOAT(landscapeNoiseLacunarity);
	CTX_LUA_FLOAT(landscapeNoiseFrequency);
	CTX_LUA_FLOAT(landscapeNoiseGain);
	CTX_LUA_INT(caveNoiseOctaves);
	CTX_LUA_FLOAT(caveNoiseLacunarity);
	CTX_LUA_FLOAT(caveNoiseFrequency);
	CTX_LUA_FLOAT(caveNoiseGain);
	CTX_LUA_FLOAT(caveDensityThreshold);
	CTX_LUA_INT(mountainNoiseOctaves);
	CTX_LUA_FLOAT(mountainNoiseLacunarity);
	CTX_LUA_FLOAT(mountainNoiseFrequency);
	CTX_LUA_FLOAT(mountainNoiseGain);

	return true;
}

}
