#include "Biome.h"
#include "commonlua/LUAFunctions.h"

template<> struct clua_meta<voxel::Biome> { static char const *name() {return "__meta_biome";} };

namespace voxel {

static int biomelua_biometostring(lua_State* s) {
	Biome** a = clua_get<Biome*>(s, 1);
	// TODO:
	lua_pushfstring(s, "temp: %f", (*a)->temperature);
	return 1;
}

static int biomelua_addtree(lua_State* s) {
	Biome** a = clua_get<Biome*>(s, 1);
	const char* treeType = luaL_checkstring(s, 2);
	const TreeType type = getTreeType(treeType);
	if (type == TreeType::Max) {
		return luaL_error(s, "Failed to resolve tree type: '%s'", treeType);
	}
	(*a)->addTreeType(type);
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
}

int biomelua_pushbiome(lua_State* s, Biome* b) {
	return clua_push(s, b);
}

}
