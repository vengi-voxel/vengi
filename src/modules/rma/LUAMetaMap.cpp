/**
 * @file
 */

#include "LUAMetaMap.h"
#include "MetaMap.h"
#include "commonlua/LUAFunctions.h"

template <> struct clua_meta<rma::MetaMap> {
	static char const *name() {
		return "__meta_metamap";
	}
};

rma::MetaMap *luametamap_tometamap(lua_State *s, int idx) {
	rma::MetaMap **metaMap = clua_get<rma::MetaMap*>(s, idx);
	if (metaMap == nullptr) {
		clua_error(s, "Unable to find meta map");
		return nullptr;
	}
	return *metaMap;
}

static int luametamap_tostring(lua_State *l) {
	rma::MetaMap *metaMap = luametamap_tometamap(l, 1);
	lua_pushfstring(l, "metamap[%s]", metaMap->name().c_str());
	Log::error("name: %s", metaMap->name().c_str());
	return 1;
}

static int luametamap_name(lua_State *l) {
	rma::MetaMap *metaMap = luametamap_tometamap(l, 1);
	lua_pushstring(l, metaMap->name().c_str());
	return 1;
}

static int luametamap_setimage(lua_State *l) {
	rma::MetaMap *metaMap = luametamap_tometamap(l, 1);
	metaMap->image = lua_tostring(l, 2);
	return 0;
}

static int luametamap_settitle(lua_State *l) {
	rma::MetaMap *metaMap = luametamap_tometamap(l, 1);
	metaMap->title = lua_tostring(l, 2);
	return 0;
}

static int luametamap_setdescription(lua_State *l) {
	rma::MetaMap *metaMap = luametamap_tometamap(l, 1);
	metaMap->description = lua_tostring(l, 2);
	return 0;
}

static int luametamap_setmodel(lua_State *l) {
	rma::MetaMap *metaMap = luametamap_tometamap(l, 1);
	metaMap->model = lua_tostring(l, 2);
	return 0;
}

static int luametamap_setsize(lua_State *l) {
	rma::MetaMap *metaMap = luametamap_tometamap(l, 1);
	metaMap->width = lua_tointeger(l, 2);
	metaMap->height = lua_tointeger(l, 3);

	if (metaMap->width < 0 || metaMap->width >= RMA_MAP_TILE_VOXEL_SIZE) {
		clua_error(l, "Invalid width given [0-%d]", RMA_MAP_TILE_VOXEL_SIZE);
	}
	if (metaMap->height < 0 || metaMap->height >= RMA_MAP_TILE_VOXEL_SIZE) {
		clua_error(l, "Invalid height given [0-%d]", RMA_MAP_TILE_VOXEL_SIZE);
	}
	return 0;
}

static int luametamap_settiles(lua_State *l) {
	const int argc = lua_gettop(l);
	clua_assert_argc(l, argc == 2);

	rma::MetaMap *metaMap = luametamap_tometamap(l, 1);
	if (!lua_istable(l, 2)) {
		clua_typerror(l, 2, "table");
	}

	lua_pushnil(l);
	while (lua_next(l, 2) != 0) {
		const char *tileId = luaL_checkstring(l, -2);
		if (tileId == nullptr || tileId[0] == '\0') {
			clua_error(l, "Empty tile id given in definition of metamap %s", metaMap->name().c_str());
		}

		rma::Tile tile;
		// Get the number of entries
		const int len = (int)lua_rawlen(l, -1);
		if (len != lengthof(tile.tiles3x3)) {
			clua_error(l, "Expected to find a 3x3 matrix as value");
		}

		for (int index = 0; index < lengthof(tile.tiles3x3); ++index) {
			// Push our target index to the stack. (lua starts at index 1!)
			lua_pushinteger(l, index + 1);
			// Get the table data at this index
			lua_gettable(l, -2);
			// Get it's value.
			const char *val = luaL_checkstring(l, -1);
			if (val == nullptr || val[0] == '\0') {
				val = "0";
			}
			tile.tiles3x3[index] = val;
			// Pop it off again.
			lua_pop(l, 1);
		}
		metaMap->tiles.put(tileId, tile);

		lua_pop(l, 1); // remove value, keep key for lua_next
	}

	return 0;
}

static int luametamap_addfixedtile(lua_State *l) {
	const int argc = lua_gettop(l);
	clua_assert_argc(l, argc == 4);

	rma::MetaMap *metaMap = luametamap_tometamap(l, 1);
	rma::FixedTile fixedTile;
	fixedTile.tileName = lua_tostring(l, 2);
	fixedTile.x = (int)luaL_optinteger(l, 3, 1);
	fixedTile.z = (int)luaL_optinteger(l, 4, 100);

	if (fixedTile.x < 0 || fixedTile.z < 0 || fixedTile.x >= metaMap->width || fixedTile.z >= metaMap->height) {
		clua_error(l, "Given fixed tile '%s' at %d:%d is out the map range 0:0-%d:%d", fixedTile.tileName.c_str(),
				   fixedTile.x, fixedTile.z, metaMap->width - 1, metaMap->height - 1);
	}

	metaMap->fixedTiles.push_back(fixedTile);
	return 0;
}

static int luametamap_addtileconfig(lua_State *l) {
	const int argc = lua_gettop(l);
	clua_assert_argc(l, argc == 3);

	rma::MetaMap *metaMap = luametamap_tometamap(l, 1);
	const char *tile = lua_tostring(l, 2);
	rma::TileConfig tileConfig;
	tileConfig.maximum = (int)luaL_optinteger(l, 3, 10);
	metaMap->tileConfigs.put(tile, tileConfig);
	return 0;
}

int luametamap_pushmetamap(lua_State *s, rma::MetaMap *b) {
	return clua_push(s, b);
}

void luametamap_setup(lua_State *s, const core::DynamicArray<luaL_Reg> &extensions) {
	core::DynamicArray<luaL_Reg> metaMapFuncs;
	metaMapFuncs.push_back({"name", luametamap_name});
	metaMapFuncs.push_back({"setImage", luametamap_setimage});
	metaMapFuncs.push_back({"addTileConfig", luametamap_addtileconfig});
	metaMapFuncs.push_back({"addFixedTile", luametamap_addfixedtile});
	metaMapFuncs.push_back({"setSize", luametamap_setsize});
	metaMapFuncs.push_back({"setTiles", luametamap_settiles});
	metaMapFuncs.push_back({"setModel", luametamap_setmodel});
	metaMapFuncs.push_back({"setTitle", luametamap_settitle});
	metaMapFuncs.push_back({"setDescription", luametamap_setdescription});
	metaMapFuncs.push_back({"__tostring", luametamap_tostring});
	for (luaL_Reg f : extensions) {
		metaMapFuncs.push_back(f);
	}
	metaMapFuncs.push_back({nullptr, nullptr});
	clua_registerfuncs(s, metaMapFuncs.data(), clua_meta<rma::MetaMap>::name());
	clua_mathregister(s);
}
