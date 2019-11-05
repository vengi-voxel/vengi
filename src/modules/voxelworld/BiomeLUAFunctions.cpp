/**
 * @file
 */
#include "BiomeManager.h"
#include "BiomeLUAFunctions.h"
#include "commonlua/LUAFunctions.h"

template<> struct clua_meta<voxelworld::Biome> { static char const *name() {return "__meta_biome";} };

namespace voxelworld {

int biomelua_setdefault(lua_State* l) {
	BiomeManager* biomeMgr = lua::LUA::globalData<BiomeManager>(l, "MGR");
	if (lua_isnil(l, 1)) {
		biomeMgr->setDefaultBiome(nullptr);
	} else {
		Biome** a = clua_get<Biome*>(l, 1);
		biomeMgr->setDefaultBiome(*a);
	}
	return 0;
}

int biomelua_addbiome(lua_State* l) {
	BiomeManager* biomeMgr = lua::LUA::globalData<BiomeManager>(l, "MGR");
	const int lower = luaL_checkinteger(l, 1);
	const int upper = luaL_checkinteger(l, 2);
	const float humidity = luaL_checknumber(l, 3);
	const float temperature = luaL_checknumber(l, 4);
	const char* voxelType = luaL_checkstring(l, 5);
	const int treeDistribution = luaL_optinteger(l, 6, 100);
	const int cloudDistribution = luaL_optinteger(l, 7, 100);
	const int plantDistribution = luaL_optinteger(l, 8, 100);
	const bool underGround = clua_optboolean(l, 9, false);
	const voxel::VoxelType type = voxel::getVoxelType(voxelType);
	if (type == voxel::VoxelType::Max) {
		return luaL_error(l, "Failed to resolve voxel type: '%s'", voxelType);
	}
	Biome* biome = biomeMgr->addBiome(lower, upper, humidity, temperature,
			type, treeDistribution, cloudDistribution, plantDistribution, underGround);
	if (biome == nullptr) {
		return luaL_error(l, "Failed to create biome");
	}
	return biomelua_pushbiome(l, biome);
}

static int biomelua_biometostring(lua_State* s) {
	Biome** a = clua_get<Biome*>(s, 1);
	// TODO:
	lua_pushfstring(s, "temp: %f", (*a)->temperature);
	return 1;
}

static int biomelua_addtree(lua_State* s) {
	Biome** a = clua_get<Biome*>(s, 1);
	const char* treeType = luaL_checkstring(s, 2);
	(*a)->addTreeType(treeType);
	lua_pushboolean(s, 1);
	return 1;
}

int biomelua_addcity(lua_State* s) {
	BiomeManager* biomeMgr = lua::LUA::globalData<BiomeManager>(s, "MGR");
	const glm::ivec2* position = clua_get<glm::ivec2>(s, 1);
	const float radius = luaL_checknumber(s, 2);
	biomeMgr->addZone(glm::ivec3(position->x, 0.0f, position->y), radius, ZoneType::City);
	lua_pushboolean(s, 1);
	return 1;
}

void biomelua_biomeregister(lua_State* s) {
	std::vector<luaL_Reg> funcs = {
		{"__tostring", biomelua_biometostring},
		{"addTree", biomelua_addtree},
		{nullptr, nullptr}
	};
	clua_registerfuncs(s, &funcs.front(), clua_meta<Biome>::name());
	clua_vecregister<glm::ivec2>(s);
	clua_vecregister<glm::ivec3>(s);
}

int biomelua_pushbiome(lua_State* s, Biome* b) {
	return clua_push(s, b);
}

}
