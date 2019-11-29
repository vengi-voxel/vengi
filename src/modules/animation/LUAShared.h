/**
 * @file
 */

#pragma once

#include "BoneId.h"
#include "commonlua/LUAFunctions.h"
#include "AnimationCache.h"

template<> struct clua_meta<animation::BoneIds> { static char const *name() {return "__meta_boneids";} };

namespace animation {

static AnimationSettings* luaGetAnimationSettings(lua_State * l) {
	return lua::LUA::globalData<AnimationSettings>(l, "Settings");
}

static int luaanim_settingssetmeshtypes(lua_State * l) {
	AnimationSettings *settings = luaGetAnimationSettings(l);
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
	AnimationSettings *settings = luaGetAnimationSettings(l);
	settings->basePath = luaL_checkstring(l, 1);
	return 0;
}

static int luaanim_settingssetpath(lua_State * l) {
	AnimationSettings *settings = luaGetAnimationSettings(l);
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

void luaanim_boneidsregister(lua_State* s) {
	std::vector<luaL_Reg> funcs = {
		{"__tostring", luaanim_boneidstostring},
		{"add", luaanim_boneidsadd},
		{nullptr, nullptr}
	};
	clua_registerfuncs(s, &funcs.front(), clua_meta<BoneIds>::name());
}

int luaanim_pushboneids(lua_State* s, BoneIds* b) {
	return clua_push(s, b);
}

int luaanim_bonesetup(lua_State* l) {
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

}
