/**
 * @file
 */

#pragma once

#include "commonlua/LUA.h"
#include "World.h"

// TODO: Map
//  users
//  npcs
// TODO: EventBus
//  onPlayerEnter
//  onPlayerLeave
// TODO: TimeProvider
//  millis
//  tickTime
// TODO: CooldownMgr
//  CooldownMgr.trigger(id, function()
//   [...]
//  end)

namespace backend {

static World* luaGetWorld(lua_State * l) {
	return lua::LUA::globalData<World>(l, "World");
}

static Map* luaGetMapContext(lua_State * l, int n) {
	return lua::LUA::userData<Map>(l, n, "Map");
}

static int luaGetMap(lua_State * l) {
	World *ctx = luaGetWorld(l);
	const MapId mapId = luaL_checkinteger(l, 1);
	const MapPtr& map = ctx->map(mapId);
	if (!map) {
		lua::LUA::returnError(l, "Could not find the map with the given id");
	}
	lua::LUA::newUserdata(l, "Map", map.get());
	return 1;
}

static int luaMapGC(lua_State * l) {
//	Map *map = luaGetMapContext(l, 1);
	// TODO: store the shared_ptr and free it here
	return 0;
}

static int luaMapToString(lua_State * l) {
	const Map *ctx = luaGetMapContext(l, 1);
	lua_pushfstring(l, "map: %i", ctx->id());
	return 1;
}

static int luaMapGetId(lua_State * l) {
	const Map *ctx = luaGetMapContext(l, 1);
	lua_pushinteger(l, ctx->id());
	return 1;
}

}
