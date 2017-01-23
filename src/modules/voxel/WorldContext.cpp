#include "WorldContext.h"
#include "commonlua/LUA.h"

namespace voxel {

#define CTX_LUA_FLOAT(name) name = lua.floatValue(#name, name)
#define CTX_LUA_INT(name) name = lua.intValue(#name, name)

WorldContext::WorldContext() :
	landscapeNoiseOctaves(1), landscapeNoisePersistence(0.1f), landscapeNoiseFrequency(0.005f), landscapeNoiseAmplitude(0.6f),
	caveNoiseOctaves(1), caveNoisePersistence(0.1f), caveNoiseFrequency(0.05f), caveNoiseAmplitude(0.1f), caveDensityThreshold(0.83f),
	mountainNoiseOctaves(2), mountainNoisePersistence(0.3f), mountainNoiseFrequency(0.00075f), mountainNoiseAmplitude(0.5f) {
}

bool WorldContext::load(const io::FilePtr& luaFile) {
	if (luaFile.get() == nullptr) {
		return false;
	}
	lua::LUA lua;
	const std::string& luaString = luaFile->load();
	if (luaString.empty()) {
		Log::error("Could not load lua script file: %s", luaFile->fileName().c_str());
		return false;
	}
	if (!lua.load(luaString)) {
		Log::error("Could not load lua script: %s. Failed with error: %s",
				luaFile->fileName().c_str(), lua.error().c_str());
		return false;
	}

	CTX_LUA_INT(landscapeNoiseOctaves);
	CTX_LUA_FLOAT(landscapeNoisePersistence);
	CTX_LUA_FLOAT(landscapeNoiseFrequency);
	CTX_LUA_FLOAT(landscapeNoiseAmplitude);
	CTX_LUA_INT(caveNoiseOctaves);
	CTX_LUA_FLOAT(caveNoisePersistence);
	CTX_LUA_FLOAT(caveNoiseFrequency);
	CTX_LUA_FLOAT(caveNoiseAmplitude);
	CTX_LUA_FLOAT(caveDensityThreshold);
	CTX_LUA_INT(mountainNoiseOctaves);
	CTX_LUA_FLOAT(mountainNoisePersistence);
	CTX_LUA_FLOAT(mountainNoiseFrequency);
	CTX_LUA_FLOAT(mountainNoiseAmplitude);

	return true;
}

}
