/**
 * @file
 */

#include "AnimationSettings.h"
#include "core/String.h"
#include "core/Assert.h"
#include "core/Array.h"
#include "core/Log.h"
#include "core/Common.h"
#include "SkeletonAttribute.h"
#include "BoneId.h"
#include "commonlua/LUAFunctions.h"

template<> struct clua_meta<animation::BoneIds> { static char const *name() {return "__meta_boneids";} };

namespace animation {

static inline AnimationSettings* luaanim_getsettings(lua_State * l) {
	return lua::LUA::globalData<AnimationSettings>(l, "Settings");
}

static int luaanim_settingssetmeshtypes(lua_State * l) {
	AnimationSettings *settings = luaanim_getsettings(l);
	const int n = lua_gettop(l);
	std::vector<std::string> types;
	types.reserve(n);
	for (int i = 1; i <= n; ++i) {
		types.push_back(luaL_checkstring(l, i));
	}
	settings->setTypes(types);
	return 0;
}

static int luaanim_settingssetbasepath(lua_State * l) {
	AnimationSettings *settings = luaanim_getsettings(l);
	settings->basePath = luaL_checkstring(l, 1);
	return 0;
}

static int luaanim_settingssetpath(lua_State * l) {
	AnimationSettings *settings = luaanim_getsettings(l);
	const char *type = luaL_checkstring(l, 1);
	const char *value = luaL_checkstring(l, 2);
	const int idx = settings->getIdxForName(type);
	if (idx < 0) {
		return luaL_error(l, "Could not find mesh type for %s", type);
	}
	settings->setPath(idx, value);
	return 0;
}

static int luaanim_boneidstostring(lua_State* s) {
	BoneIds** a = clua_get<BoneIds*>(s, 1);
	BoneIds& boneIds = **a;
	if (boneIds.num == 0) {
		lua_pushstring(s, "empty");
		return 1;
	}
	if (boneIds.num == 1) {
		lua_pushfstring(s, "num bones: 1, bone[0]: %i", (int)boneIds.bones[0]);
		return 1;
	}
	if (boneIds.num == 2) {
		lua_pushfstring(s, "num bones: 2, bone[0]: %i, bone[1]: %i", (int)boneIds.bones[0], (int)boneIds.bones[1]);
		return 1;
	}
	lua_pushfstring(s, "error: num bones: %i", (*a)->num);
	return 1;
}

static int luaanim_boneidsadd(lua_State* s) {
	BoneIds** boneIdsPtrPtr = clua_get<BoneIds*>(s, 1);
	BoneIds* boneIds = *boneIdsPtrPtr;
	const char* boneName = luaL_checkstring(s, 2);
	BoneId id = toBoneId(boneName);
	if (id == BoneId::Max) {
		return luaL_error(s, "Failed to resolve bone: '%s'", boneName);
	}
	if (boneIds->num > lengthof(boneIds->bones)) {
		lua_pushboolean(s, 0);
		return 1;
	}
	boneIds->bones[boneIds->num] = (uint8_t)id;
	boneIds->mirrored[boneIds->num] = clua_optboolean(s, 3, false);
	boneIds->num++;
	lua_pushboolean(s, 1);
	return 1;
}

static int luaanim_pushboneids(lua_State* s, BoneIds* b) {
	return clua_push(s, b);
}

static int luaanim_bonesetup(lua_State* l) {
	AnimationSettings* settings = lua::LUA::globalData<AnimationSettings>(l, "Settings");
	const char* meshType = luaL_checkstring(l, 1);
	const int idx = settings->getIdxForName(meshType);
	if (idx < 0 || idx >= (int)AnimationSettings::MAX_ENTRIES) {
		return luaL_error(l, "Could not find mesh type for %s", meshType);
	}
	BoneIds& b = settings->boneIds(idx);
	b = BoneIds {};
	if (luaanim_pushboneids(l, &b) != 1) {
		return luaL_error(l, "Failed to push the bonesids");
	}
	return 1;
}

static constexpr luaL_Reg settingsFuncs[] = {
	{ "setBasePath", luaanim_settingssetbasepath },
	{ "setPath", luaanim_settingssetpath },
	{ "setMeshTypes", luaanim_settingssetmeshtypes },
	{ nullptr, nullptr }
};

static constexpr luaL_Reg globalBoneFuncs[] = {
	{ "setup", luaanim_bonesetup },
	{ nullptr, nullptr }
};

static constexpr luaL_Reg boneFuncs[] = {
	{"__tostring", luaanim_boneidstostring},
	{"add", luaanim_boneidsadd},
	{nullptr, nullptr}
};

std::string luaFilename(const char *character) {
	return core::string::format("%s.lua", character);
}

bool loadAnimationSettings(const std::string& luaString, AnimationSettings& settings, void* skeletonAttr, const SkeletonAttributeMeta* metaIter) {
	if (luaString.empty()) {
		Log::warn("empty animation settings can't get loaded");
		return false;
	}

	lua::LUA lua;
	lua.reg("settings", settingsFuncs);
	lua.reg("bone", globalBoneFuncs);
	clua_registerfuncs(lua.state(), boneFuncs, clua_meta<BoneIds>::name());

	if (!lua.load(luaString)) {
		Log::error("%s", lua.error().c_str());
		return false;
	}

	lua.newGlobalData<AnimationSettings>("Settings", &settings);
	if (!lua.execute("init", LUA_MULTRET)) {
		Log::error("%s", lua.error().c_str());
		return false;
	}

	for (; metaIter->name; ++metaIter) {
		const SkeletonAttributeMeta& meta = *metaIter;
		float *saVal = (float*)(((uint8_t*)skeletonAttr) + meta.offset);
		if (lua.valueFloatFromTable(meta.name, saVal)) {
			Log::debug("Skeleton attribute value for %s: %f", meta.name, *saVal);
		} else {
			Log::debug("Skeleton attribute value for %s not given - use default: %f", meta.name, *saVal);
		}
	}

	return true;
}


void AnimationSettings::setTypes(const std::vector<std::string>& types) {
	_types = types;
}

const std::string& AnimationSettings::type(size_t idx) const {
	if (idx >= (int) MAX_ENTRIES) {
		static const std::string EMPTY;
		return EMPTY;
	}
	return _types[idx];
}

int AnimationSettings::getIdxForName(const char *name) const {
	for (size_t i = 0; i < _types.size(); ++i) {
		if (_types[i] == name) {
			return i;
		}
	}
	return -1;
}

std::string AnimationSettings::fullPath(int idx, const char *name) const {
	if (idx < 0 || idx >= (int) MAX_ENTRIES) {
		static const std::string EMPTY;
		return EMPTY;
	}
	if (name == nullptr) {
		name = paths[idx].c_str();
	}
	return core::string::format("%s/%s/%s.vox", basePath.c_str(), _types[idx].c_str(), name);
}

std::string AnimationSettings::path(int idx, const char *name) const {
	if (idx < 0 || idx >= (int) MAX_ENTRIES) {
		static const std::string EMPTY;
		return EMPTY;
	}
	if (name == nullptr) {
		name = paths[idx].c_str();
	}
	return core::string::format("%s/%s", _types[idx].c_str(), name);
}

bool AnimationSettings::setPath(int idx, const char *str) {
	core_assert(idx >= 0 && idx < (int) MAX_ENTRIES);
	paths[idx] = str;
	return true;
}

const BoneIds& AnimationSettings::boneIds(int idx) const {
	core_assert(idx >= 0 && idx < (int) MAX_ENTRIES);
	return boneIdsArray[idx];
}

BoneIds& AnimationSettings::boneIds(int idx) {
	core_assert(idx >= 0 && idx < (int) MAX_ENTRIES);
	return boneIdsArray[idx];
}

}
